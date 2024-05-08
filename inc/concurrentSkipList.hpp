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

template <typename T, typename K, unsigned maxLevel>
struct ConcurrentSkipList {
   public:
    ConcurrentSkipList();
    int generateRandomLevel();
    std::shared_ptr<Node<T, K>> createNode(T, K, int);

    bool searchElement(T);
    bool insertElement(T, K);
    bool deleteElement(T);
    void displayList();
    size_t size();

   private:
    std::atomic<int> currentLevel{0};
    std::atomic<size_t> elementsCount{0};

    std::shared_ptr<Node<T, K>> head;
    std::shared_ptr<Node<T, K>> tail;

    int searchHelper(T key, std::vector<std::shared_ptr<Node<T, K>>>& preds,
                     std::vector<std::shared_ptr<Node<T, K>>>& succs);
};

template <typename T, typename K, unsigned maxLevel>
size_t ConcurrentSkipList<T, K, maxLevel>::size() {
    return this->elementsCount;
}

template <typename T, typename K, unsigned maxLevel>
ConcurrentSkipList<T, K, maxLevel>::ConcurrentSkipList() {
    head = createNode(std::numeric_limits<T>::min(), 0, maxLevel);
    tail = createNode(std::numeric_limits<T>::max(), 0, maxLevel);
    for (int level = 0; level <= static_cast<int>(maxLevel); level++) {
        head->forward.at(level) = tail;
    }
}

template <typename T, typename K, unsigned maxLevel>
int ConcurrentSkipList<T, K, maxLevel>::generateRandomLevel() {
    return rand() % maxLevel;
}

template <typename T, typename K, unsigned maxLevel>
std::shared_ptr<Node<T, K>> ConcurrentSkipList<T, K, maxLevel>::createNode(
    T key, K value, int level) {
    return std::make_shared<Node<T, K>>(key, value, level);
}

template <typename T, typename K, unsigned maxLevel>
int ConcurrentSkipList<T, K, maxLevel>::searchHelper(
    T key, std::vector<std::shared_ptr<Node<T, K>>>& preds,
    std::vector<std::shared_ptr<Node<T, K>>>& succs) {
    int levelFound = -1;
    std::shared_ptr<Node<T, K>> pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        std::shared_ptr<Node<T, K>> curr = pred->forward.at(level);
        while (key > curr->getKey()) {
            pred = curr;
            curr = pred->forward.at(level);
        }
        if (levelFound == -1 && key == curr->getKey()) {
            levelFound = level;
        }
        preds[level] = pred;
        succs[level] = curr;
    }

    return levelFound;
}

template <typename T, typename K, unsigned maxLevel>
bool ConcurrentSkipList<T, K, maxLevel>::searchElement(T key) {
    std::vector<std::shared_ptr<Node<T, K>>> preds(maxLevel + 1);
    std::vector<std::shared_ptr<Node<T, K>>> succs(maxLevel + 1);

    int levelFound = searchHelper(key, preds, succs);

    return (levelFound != -1 && succs[levelFound]->fullyLinked &&
            !succs[levelFound]->marked);
}

template <typename T, typename K, unsigned maxLevel>
bool ConcurrentSkipList<T, K, maxLevel>::insertElement(T key, K value) {
    int topLevel = generateRandomLevel();
    std::vector<std::shared_ptr<Node<T, K>>> preds(maxLevel + 1);
    std::vector<std::shared_ptr<Node<T, K>>> succs(maxLevel + 1);

    while (true) {
        int levelFound = searchHelper(key, preds, succs);
        if (levelFound != -1) {
            std::shared_ptr<Node<T, K>> nodeFound = succs.at(levelFound);
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
            std::shared_ptr<Node<T, K>> pred = preds[level];
            std::shared_ptr<Node<T, K>> succ = succs[level];
            pred->lock();
            highestLocked = level;
            valid = !pred->marked && !succ->marked &&
                    pred->forward.at(level) == succ;
        }
        if (valid) {
            std::shared_ptr<Node<T, K>> newNode =
                createNode(key, value, topLevel);
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

template <typename T, typename K, unsigned maxLevel>
bool ConcurrentSkipList<T, K, maxLevel>::deleteElement(T key) {
    std::shared_ptr<Node<T, K>> victim = nullptr;
    bool isMarked = false;
    int topLevel = -1;
    std::vector<std::shared_ptr<Node<T, K>>> preds(maxLevel + 1);
    std::vector<std::shared_ptr<Node<T, K>>> succs(maxLevel + 1);

    while (true) {
        int levelFound = searchHelper(key, preds, succs);
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
            std::shared_ptr<Node<T, K>> pred, succ;
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

template <typename T, typename K, unsigned maxLevel>
void ConcurrentSkipList<T, K, maxLevel>::displayList() {
    std::cout << "Concurrent Skip-List" << std::endl;

    if (head->forward.at(0) == nullptr) {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(this->elementsCount);

    std::vector<std::vector<std::string>> builder(
        len, std::vector<std::string>(this->currentLevel + 1));

    std::shared_ptr<Node<T, K>> current = head->forward.at(0);

    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < this->currentLevel + 1; ++j) {
            if (j < static_cast<int>(current->forward.size())) {
                std::ostringstream oss;
                oss << std::setw(2) << std::setfill('0') << current->getKey();
                builder.at(i).at(j) = oss.str();
            } else {
                builder.at(i).at(j) = "--";
            }
        }
        current = current->forward.at(0);
    }

    for (int i = this->currentLevel; i >= 0; --i) {
        std::cout << "Level " << i << ": head ";
        for (int j = 0; j < len; ++j) {
            if (builder.at(j).at(i) == "--") {
                std::cout << "------";
            } else {
                std::cout << "--->" << builder.at(j).at(i);
            }
        }
        std::cout << "--->Null" << std::endl;
    }
}