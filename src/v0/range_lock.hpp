#pragma once
#include <atomic>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

#include "node.hpp"

template <typename T, unsigned maxLevel>
struct ConcurrentRangeLock {
   public:
    ConcurrentRangeLock();
    ~ConcurrentRangeLock();
    unsigned generateRandomLevel();
    Node<T> *createNode(T, T, int);

    bool searchLock(T, T);
    bool tryLock(T, T);
    bool releaseLock(T, T);
    void displayList();
    size_t size();

   private:
    int currentLevel;
    std::atomic<size_t> elementsCount{0};

    Node<T> *head;
    Node<T> *tail;
    Node<T> *removed = nullptr;

    int findInsert(T start, T end, Node<T> **preds, Node<T> **succs);
    int findExact(T start, T end, Node<T> **preds, Node<T> **succs);
};

template <typename T, unsigned maxLevel>
size_t ConcurrentRangeLock<T, maxLevel>::size() {
    return this->elementsCount.load(std::memory_order_relaxed);
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock<T, maxLevel>::ConcurrentRangeLock() {
    std::srand(std::time(0));

    auto min = std::numeric_limits<T>::min();
    auto max = std::numeric_limits<T>::max();

    head = createNode(min, min, maxLevel);
    tail = createNode(max, max, maxLevel);

    for (unsigned level = 0; level <= maxLevel; ++level) {
        head->next[level] = tail;
    }
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock<T, maxLevel>::~ConcurrentRangeLock() {
    // Node<T> *curr = head;
    // while (curr != nullptr)
    // {
    //     Node<T> *next = curr->next[0];
    //     delete curr;
    //     curr = next;
    // }

    // curr = removed;
    // while (curr != nullptr)
    // {
    //     Node<T> *next = curr->removed;
    //     delete curr;
    //     curr = next;
    // }
}

template <typename T, unsigned maxLevel>
unsigned ConcurrentRangeLock<T, maxLevel>::generateRandomLevel() {
    float randNum = static_cast<float>(rand() / RAND_MAX);

    float threshold = 0.5;  // Probability for level 1
    unsigned level = 1;

    while (randNum > threshold && level < maxLevel) {
        randNum -= threshold;
        threshold /= 2;
        level++;
    }

    return level;
}

template <typename T, unsigned maxLevel>
Node<T> *ConcurrentRangeLock<T, maxLevel>::createNode(T start, T end,
                                                      int level) {
    return new Node<T>(start, end, level);
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::findInsert(T start, T end,
                                                 Node<T> **preds,
                                                 Node<T> **succs) {
    int lFound = -1;
    Node<T> *pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        Node<T> *curr = pred->next[level];

        while (start > curr->end) {
            pred = curr;
            curr = pred->next[level];
        }

        if (lFound == -1 && end >= curr->start) {
            lFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return lFound;
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::findExact(T start, T end, Node<T> **preds,
                                                Node<T> **succs) {
    int lFound = -1;
    Node<T> *pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        Node<T> *curr = pred->next[level];

        while (start > curr->end) {
            pred = curr;
            curr = pred->next[level];
        }

        if (lFound == -1 && start == curr->start && end == curr->end) {
            lFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return lFound;
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::searchLock(T start, T end) {
    Node<T> *preds[maxLevel + 1];
    Node<T> *succs[maxLevel + 1];

    int lFound = findExact(start, end, preds, succs);

    return (lFound != -1 && succs[lFound]->fullyLinked &&
            !succs[lFound]->marked);
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::tryLock(T start, T end) {
    int topL = static_cast<int>(generateRandomLevel());
    Node<T> *preds[maxLevel + 1];
    Node<T> *succs[maxLevel + 1];

    std::set<Node<T> *> lockedNodes;

    while (true) {
        ScopeGuard unlockGuard([&lockedNodes]() {
            for (auto lockedNode : lockedNodes) {
                lockedNode->unlock();
            }
            lockedNodes.clear();
        });

        int lFound = findInsert(start, end, preds, succs);
        if (lFound != -1) {
            Node<T> *nodeFound = succs[lFound];
            if (!nodeFound->marked) {
                while (!nodeFound->fullyLinked) {
                    std::this_thread::yield();
                }
                return false;
            }
            continue;
        }

        int highestLocked = -1;
        bool valid = true;

        for (int level = 0; valid && level <= topL; level++) {
            Node<T> *pred = preds[level];
            Node<T> *succ = succs[level];
            if (lockedNodes.find(pred) == lockedNodes.end()) {
                pred->lock();
                lockedNodes.insert(pred);
            }
            highestLocked = level;
            valid =
                !pred->marked && !succ->marked && (pred->next[level] == succ);
        }

        if (!valid) continue;

        Node<T> *newNode = createNode(start, end, topL);
        for (int level = 0; level <= topL; level++) {
            newNode->next[level] = succs[level];
            preds[level]->next[level] = newNode;
        }
        newNode->fullyLinked = true;

        currentLevel = topL > currentLevel ? topL : currentLevel;
        elementsCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::releaseLock(T start, T end) {
    Node<T> *victim = nullptr;
    bool isMarked = false;
    int topL = -1;

    std::set<Node<T> *> lockedNodes;

    Node<T> *preds[maxLevel + 1];
    Node<T> *succs[maxLevel + 1];

    while (true) {
        int lFound = findExact(start, end, preds, succs);
        if (lFound != -1) victim = succs[lFound];

        if (isMarked ||
            (lFound != -1 && victim->topL() == lFound && !victim->marked)) {
            ScopeGuard unlockGuard([&lockedNodes]() {
                for (auto lockedNode : lockedNodes) {
                    lockedNode->unlock();
                }
                lockedNodes.clear();
            });

            if (!isMarked) {
                topL = victim->topL();

                victim->lockR();

                if (victim->marked) {
                    victim->unlockR();
                    return false;
                }

                victim->marked = true;
                isMarked = true;
            }

            int highestLocked = -1;
            bool valid = true;
            Node<T> *pred, *succ;

            for (int level = 0; valid && level <= topL; level++) {
                pred = preds[level];
                if (lockedNodes.find(pred) == lockedNodes.end()) {
                    pred->lock();
                    lockedNodes.insert(pred);
                }
                highestLocked = level;
                valid = !pred->marked && pred->next[level] == victim;
            }

            if (!valid) continue;

            for (int level = topL; level >= 0; level--) {
                preds[level]->next[level] = victim->next[level];
            }

            victim->unlockR();

            elementsCount.fetch_sub(1, std::memory_order_relaxed);

            return true;
        } else {
            return false;
        }
    }
}

template <typename T, unsigned maxLevel>
void ConcurrentRangeLock<T, maxLevel>::displayList() {
    std::cout << "Concurrent Range Lock" << std::endl;

    if (head->next[0] == tail) {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(this->elementsCount);

    std::vector<std::vector<std::string>> builder(
        len, std::vector<std::string>(this->currentLevel + 1));

    Node<T> *current = head->next[0];

    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < this->currentLevel + 1; ++j) {
            if (j < current->topL() + 1) {
                std::ostringstream oss;
                oss << "[" << std::setw(2) << std::setfill('0')
                    << current->start << "," << std::setw(2)
                    << std::setfill('0') << current->end << "]";
                builder[i][j] = oss.str();
            } else {
                builder[i][j] = "---------";
            }
        }
        current = current->next[0];
    }

    for (int i = this->currentLevel; i >= 0; --i) {
        std::cout << "Level " << i << ": head ";
        for (int j = 0; j < len; ++j) {
            if (builder[j][i] == "---------") {
                std::cout << "---------";
            } else {
                std::cout << "->" << builder[j][i];
            }
        }
        std::cout << "---> tail" << std::endl;
    }
}
