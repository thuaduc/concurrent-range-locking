#pragma once
#include <atomic>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "node.hpp"

class ScopeGuard {
   public:
    explicit ScopeGuard(std::function<void()> onExitScope)
        : onExitScope_(onExitScope) {}

    ~ScopeGuard() { onExitScope_(); }

   private:
    std::function<void()> onExitScope_;
};

template <typename T>
class Node_V1Locker {
   public:
    void trackAndLock(Node_V1<T> *Node_V1) {
        // Lock the Node_V1 if it's not already tracked and locked
        if (std::find(trackedNode_V1s.begin(), trackedNode_V1s.end(),
                      Node_V1) == trackedNode_V1s.end()) {
            Node_V1->lock();
            trackedNode_V1s.push_back(Node_V1);
        }
    }

    void unlockAll() {
        // Unlock all tracked Node_V1s in reverse order
        for (auto it = trackedNode_V1s.rbegin(); it != trackedNode_V1s.rend();
             ++it) {
            (*it)->unlock();
        }
        // Clear the vector after unlocking
        trackedNode_V1s.clear();
    }

   private:
    std::vector<Node_V1<T> *> trackedNode_V1s;
};

template <typename T, unsigned maxLevel>
struct ConcurrentRangeLock_V1 {
   public:
    ConcurrentRangeLock_V1();
    ~ConcurrentRangeLock_V1();
    unsigned generateRandomLevel();
    Node_V1<T> *createNode_V1(T, T, int);

    bool searchLock(T, T);
    bool tryLock(T, T);
    bool releaseLock(T, T);
    void displayList();
    size_t size();

   private:
    unsigned currentLevel{maxLevel};
    std::atomic<size_t> elementsCount{0};

    Node_V1<T> *head;
    Node_V1<T> *tail;

    int findInsert(T start, T end, Node_V1<T> **preds, Node_V1<T> **succs);
    int findExact(T start, T end, Node_V1<T> **preds, Node_V1<T> **succs);
};

template <typename T, unsigned maxLevel>
size_t ConcurrentRangeLock_V1<T, maxLevel>::size() {
    return this->elementsCount.load(std::memory_order_relaxed);
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock_V1<T, maxLevel>::ConcurrentRangeLock_V1() {
    std::srand(std::time(0));

    auto min = std::numeric_limits<T>::min();
    auto max = std::numeric_limits<T>::max();

    head = createNode_V1(min, min, maxLevel);
    tail = createNode_V1(max, max, maxLevel);

    for (unsigned level = 0; level <= maxLevel; ++level) {
        head->next[level] = tail;
    }
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock_V1<T, maxLevel>::~ConcurrentRangeLock_V1() {}

template <typename T, unsigned maxLevel>
unsigned ConcurrentRangeLock_V1<T, maxLevel>::generateRandomLevel() {
    unsigned level = 1;
    while (rand() % 2 == 0 && level < maxLevel) {
        ++level;
    }
    return level;
}

template <typename T, unsigned maxLevel>
Node_V1<T> *ConcurrentRangeLock_V1<T, maxLevel>::createNode_V1(T start, T end,
                                                               int level) {
    return new Node_V1<T>(start, end, level);
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock_V1<T, maxLevel>::findInsert(T start, T end,
                                                    Node_V1<T> **preds,
                                                    Node_V1<T> **succs) {
    int levelFound = -1;
    Node_V1<T> *pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        Node_V1<T> *curr = pred->next[level];

        while (start >= curr->getEnd()) {
            pred = curr;
            curr = pred->next[level];
        }

        if (levelFound == -1 && end >= curr->getStart()) {
            levelFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return levelFound;
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock_V1<T, maxLevel>::findExact(T start, T end,
                                                   Node_V1<T> **preds,
                                                   Node_V1<T> **succs) {
    int levelFound = -1;
    Node_V1<T> *pred = head;

    for (int level = maxLevel; level >= 0; level--) {
        Node_V1<T> *curr = pred->next[level];

        while (start >= curr->getEnd()) {
            pred = curr;
            curr = pred->next[level];
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

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock_V1<T, maxLevel>::searchLock(T start, T end) {
    Node_V1<T> *preds[maxLevel + 1];
    Node_V1<T> *succs[maxLevel + 1];

    int levelFound = findExact(start, end, preds, succs);

    return (levelFound != -1 && succs[levelFound]->fullyLinked &&
            !succs[levelFound]->marked);
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock_V1<T, maxLevel>::tryLock(T start, T end) {
    const auto topLevel = generateRandomLevel();
    Node_V1<T> *preds[maxLevel + 1];
    Node_V1<T> *succs[maxLevel + 1];

    while (true) {
        int levelFound = findInsert(start, end, preds, succs);
        if (levelFound != -1) {
            Node_V1<T> *Node_V1Found = succs[levelFound];
            if (!Node_V1Found->marked) {
                return false;
            }
            // std::this_thread::yield();
            continue;
        }

        bool valid = true;
        Node_V1Locker<T> Node_V1Locker;
        ScopeGuard unlockGuard(
            [&Node_V1Locker]() { Node_V1Locker.unlockAll(); });

        for (int level = 0; valid && (level <= topLevel); ++level) {
            Node_V1<T> *pred = preds[level];
            Node_V1<T> *succ = succs[level];

            Node_V1Locker.trackAndLock(pred);

            valid = !pred->marked && !succ->marked && pred->next[level] == succ;
        }

        if (!valid) {
            continue;
        }

        Node_V1<T> *newNode_V1 = createNode_V1(start, end, topLevel);
        for (int level = 0; level <= topLevel; ++level) {
            newNode_V1->next[level] = succs[level];
            preds[level]->next[level] = newNode_V1;
        }
        newNode_V1->fullyLinked = true;

        elementsCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
}
template <typename T, unsigned maxLevel>

bool ConcurrentRangeLock_V1<T, maxLevel>::releaseLock(T start, T end) {
    Node_V1<T> *victim = nullptr;
    bool isMarked = false;
    int topLevel = -1;

    Node_V1<T> *preds[maxLevel + 1];
    Node_V1<T> *succs[maxLevel + 1];

    while (true) {
        Node_V1Locker<T> Node_V1Locker;
        ScopeGuard unlockGuard(
            [&Node_V1Locker]() { Node_V1Locker.unlockAll(); });

        int levelFound = findExact(start, end, preds, succs);
        if (levelFound != -1) {
            victim = succs[levelFound];
        } else {
            std::cerr << "Wrong usage of releaseLock" << std::endl;
            exit(0);
        }

        if (isMarked ||
            (levelFound != -1 && victim->getTopLevel() == levelFound &&
             !victim->marked)) {
            if (!isMarked) {
                topLevel = victim->getTopLevel();

                Node_V1Locker.trackAndLock(victim);

                if (victim->marked) {
                    return false;
                }
                victim->marked = true;
                isMarked = true;
            }

            bool valid = true;
            Node_V1<T> *pred, *succ;

            for (int level = 0; valid && level <= topLevel; ++level) {
                pred = preds[level];
                Node_V1Locker.trackAndLock(pred);
                valid = !pred->marked && pred->next[level] == victim;
            }

            if (!valid) {
                // std::this_thread::yield();
                continue;
            }

            for (int level = topLevel; level >= 0; --level) {
                preds[level]->next[level] = victim->next[level];
            }

            elementsCount.fetch_sub(1, std::memory_order_relaxed);

            return true;
        } else {
            std::cout << isMarked << levelFound << victim->getTopLevel()
                      << levelFound << victim->marked << std::endl;
            return false;
        }
    }
}
template <typename T, unsigned maxLevel>

void ConcurrentRangeLock_V1<T, maxLevel>::displayList() {
    std::cout << "Concurrent Range Lock" << std::endl;

    if (head->next[0] == nullptr) {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(this->elementsCount);

    std::vector<std::vector<std::string>> builder(
        len, std::vector<std::string>(this->currentLevel + 1));

    Node_V1<T> *current = head->next[0];

    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < this->currentLevel + 1; ++j) {
            if (j < current->getTopLevel() + 1) {
                std::ostringstream oss;
                oss << "[" << std::setw(2) << std::setfill('0')
                    << current->getStart() << "," << std::setw(2)
                    << std::setfill('0') << current->getEnd() << "]";
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

template <typename T>
void trackAndLock(Node_V1<T> *pred, std::vector<Node_V1<T> *> &toUnlock) {
    if (std::find(toUnlock.begin(), toUnlock.end(), pred) == toUnlock.end()) {
        pred->lock();
        toUnlock.push_back(pred);
    }
}