#pragma once
#include <atomic>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

#include "node.hpp"

template <typename T, unsigned maxLevel>
struct ConcurrentRangeLock {
   public:
    ConcurrentRangeLock();
    int generateRandomLevel();
    std::shared_ptr<Node<T, T>> createNode(T, T, int);

    bool searchLock(T, T);
    bool tryLock(T, T);
    bool releaseLock(T, T);
    void displayList();
    size_t size();

   private:
    std::atomic<int> currentLevel{0};
    std::atomic<size_t> elementsCount{0};

    std::shared_ptr<Node<T, T>> head;
    std::shared_ptr<Node<T, T>> tail;

    int findInsert(T start, T end,
                   std::vector<std::shared_ptr<Node<T, T>>>& preds,
                   std::vector<std::shared_ptr<Node<T, T>>>& succs);

    int findExact(T start, T end,
                  std::vector<std::shared_ptr<Node<T, T>>>& preds,
                  std::vector<std::shared_ptr<Node<T, T>>>& succs);
};

template <typename T, unsigned maxLevel>
size_t ConcurrentRangeLock<T, maxLevel>::size() {
    return this->elementsCount;
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock<T, maxLevel>::ConcurrentRangeLock() {
    auto min = std::numeric_limits<T>::min();
    auto max = std::numeric_limits<T>::max();

    head = createNode(min, min, maxLevel);
    tail = createNode(max, max, maxLevel);

    for (int level = 0; level <= static_cast<int>(maxLevel); level++) {
        head->forward.at(level) = tail;
    }
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::generateRandomLevel() {
    return rand() % maxLevel;
}

template <typename T, unsigned maxLevel>
std::shared_ptr<Node<T, T>> ConcurrentRangeLock<T, maxLevel>::createNode(
    T start, T end, int level) {
    return std::make_shared<Node<T, T>>(start, end, level);
}

/**
 * @brief Searches for a node position to be inserted in the skip list
 *        Modify the preds and succs array
 *
 * @param start The start value to search for.
 * @param preds An array of predecessors at each level of the skip list.
 * @param succs An array of successors at each level of the skip list.
 *
 * @return The level at which the node is found (-1 if not found).
 */
template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::findInsert(
    T start, T end, std::vector<std::shared_ptr<Node<T, T>>>& preds,
    std::vector<std::shared_ptr<Node<T, T>>>& succs) {
    int levelFound = -1;
    std::shared_ptr<Node<T, T>> pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        std::shared_ptr<Node<T, T>> curr = pred->forward.at(level);

        while (start > curr->getEnd()) {
            pred = curr;
            curr = pred->forward.at(level);
        }

        if (levelFound == -1 && end >= curr->getStart()) {
            levelFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return levelFound;
}

/**
 * @brief Searches for an exact node in the skip list
 *        Modify the preds and succs array
 *
 * @param start The start value to search for.
 * @param preds An array of predecessors at each level of the skip list.
 * @param succs An array of successors at each level of the skip list.
 *
 * @return The level at which the node is found (-1 if not found).
 */
template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::findExact(
    T start, T end, std::vector<std::shared_ptr<Node<T, T>>>& preds,
    std::vector<std::shared_ptr<Node<T, T>>>& succs) {
    int levelFound = -1;
    std::shared_ptr<Node<T, T>> pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        std::shared_ptr<Node<T, T>> curr = pred->forward.at(level);

        while (start > curr->getEnd()) {
            pred = curr;
            curr = pred->forward.at(level);
        }

        if (levelFound == -1 && start == curr->getStart() &&
            end == curr->getEnd()) {
            levelFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return levelFound;
}

/**
 * @brief Searches for the range of a given start value
 *
 * @param start The start value to search for.
 *
 * @return Return if range is found, if node is fully liked and marked
 */
template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::searchLock(T start, T end) {
    std::vector<std::shared_ptr<Node<T, T>>> preds(maxLevel + 1);
    std::vector<std::shared_ptr<Node<T, T>>> succs(maxLevel + 1);

    int levelFound = findExact(start, end, preds, succs);

    return (levelFound != -1 && succs[levelFound]->fullyLinked &&
            !succs[levelFound]->marked);
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::tryLock(T start, T end) {
    int topLevel = generateRandomLevel();
    std::vector<std::shared_ptr<Node<T, T>>> preds(maxLevel + 1);
    std::vector<std::shared_ptr<Node<T, T>>> succs(maxLevel + 1);

    while (true) {
        int levelFound = findInsert(start, end, preds, succs);
        if (levelFound != -1) {
            std::shared_ptr<Node<T, T>> nodeFound = succs.at(levelFound);
            if (!nodeFound->marked) {
                while (!nodeFound->fullyLinked) {
                };
                return false;
            }
            continue;
        }
        int highestLocked = -1;
        bool valid = true;
        for (int level = 0; valid && level <= topLevel; level++) {
            std::shared_ptr<Node<T, T>> pred = preds[level];
            std::shared_ptr<Node<T, T>> succ = succs[level];
            pred->lock();
            highestLocked = level;
            valid = !pred->marked && !succ->marked &&
                    pred->forward.at(level) == succ;
        }
        if (valid) {
            std::shared_ptr<Node<T, T>> newNode =
                createNode(start, end, topLevel);
            for (int level = 0; level <= topLevel; level++) {
                newNode->forward.at(level) = succs[level];
                preds[level]->forward.at(level) = newNode;
            }
            newNode->fullyLinked = true;
        }

        for (int level = 0; level <= highestLocked; level++) {
            preds[level]->unlock();
        }

        if (!valid) {
            continue;
        } else {
            if (topLevel > currentLevel.load(std::memory_order_relaxed)) {
                currentLevel.store(topLevel, std::memory_order_relaxed);
            }
            elementsCount.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::releaseLock(T start, T end) {
    std::shared_ptr<Node<T, T>> victim = nullptr;
    bool isMarked = false;
    int topLevel = -1;
    std::vector<std::shared_ptr<Node<T, T>>> preds(maxLevel + 1);
    std::vector<std::shared_ptr<Node<T, T>>> succs(maxLevel + 1);

    while (true) {
        int levelFound = findExact(start, end, preds, succs);
        if (levelFound != -1) {
            victim = succs.at(levelFound);
        }
        if (isMarked |
            (levelFound != -1 &&
             static_cast<int>(victim->forward.size() - 1) == levelFound &&
             !victim->marked)) {
            if (!isMarked) {
                topLevel = victim->forward.size() - 1;
                victim->lock();
                if (victim->marked) {
                    victim->unlock();
                    return false;
                }
                victim->marked = true;
                isMarked = true;
            }
            int highestLocked = -1;
            bool valid = true;
            std::shared_ptr<Node<T, T>> pred, succ;
            for (int level = 0; valid && level <= topLevel; level++) {
                pred = preds.at(level);
                pred->lock();
                highestLocked = level;
                valid = !pred->marked && pred->forward.at(level) == victim;
            }
            if (valid) {
                for (int level = topLevel; level >= 0; level--) {
                    preds.at(level)->forward.at(level) =
                        victim->forward.at(level);
                }
                victim->unlock();
            }

            for (int level = highestLocked; level >= 0; level--) {
                preds.at(level)->unlock();
            }

            if (valid) {
                elementsCount.fetch_sub(1, std::memory_order_relaxed);
                return true;
            }
        } else
            return false;
    }
}

template <typename T, unsigned maxLevel>
void ConcurrentRangeLock<T, maxLevel>::displayList() {
    std::cout << "Concurrent Range Lock" << std::endl;

    if (head->forward.at(0) == nullptr) {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(this->elementsCount);

    std::vector<std::vector<std::string>> builder(
        len, std::vector<std::string>(this->currentLevel + 1));

    std::shared_ptr<Node<T, T>> current = head->forward.at(0);

    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < this->currentLevel + 1; ++j) {
            if (j < static_cast<int>(current->forward.size())) {
                std::ostringstream oss;
                oss << "[" << std::setw(2) << std::setfill('0')
                    << current->getStart() << "," << std::setw(2)
                    << std::setfill('0') << current->getEnd() << "]";
                builder.at(i).at(j) = oss.str();
            } else {
                builder.at(i).at(j) = "---------";
            }
        }
        current = current->forward.at(0);
    }

    for (int i = this->currentLevel; i >= 0; --i) {
        std::cout << "Level " << i << ": head ";
        for (int j = 0; j < len; ++j) {
            if (builder.at(j).at(i) == "---------") {
                std::cout << "---------";
            } else {
                std::cout << "->" << builder.at(j).at(i);
            }
        }
        std::cout << "---> tail" << std::endl;
    }
}