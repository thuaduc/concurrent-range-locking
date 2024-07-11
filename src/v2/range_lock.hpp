#include <atomic>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "node.hpp"

template <typename T, unsigned maxLevel>
class ConcurrentRangeLock {
   private:
    Node<T>* head;
    Node<T>* tail;
    std::atomic<size_t> elementsCount{0};

    int randomLevel();
    bool find(const T& start, const T& end, Node<T>** preds, Node<T>** succs);
    bool findExact(const T& start, const T& end, Node<T>** preds,
                   Node<T>** succs);

   public:
    ConcurrentRangeLock();
    bool tryLock(T start, T end);
    bool releaseLock(T start, T end);

    size_t size();
    void displayList();
};

template <typename T, unsigned maxLevel>
ConcurrentRangeLock<T, maxLevel>::ConcurrentRangeLock() {
    auto min = std::numeric_limits<T>::min();
    auto max = std::numeric_limits<T>::max();

    head = new Node<T>();
    tail = new Node<T>();

    tail->initialize(max, max, maxLevel);
    head->initializeHead(min, min, maxLevel, tail);

    srand(0);
}

template <typename T, unsigned maxLevel>
size_t ConcurrentRangeLock<T, maxLevel>::size() {
    return elementsCount.load();
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::randomLevel() {
    int level = 0;
    while (rand() % 2 && level < maxLevel) {
        level++;
    }
    return level;
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::find(const T& start, const T& end,
                                            Node<T>** preds, Node<T>** succs) {
    int bottomLevel = 0;
    bool marked[1] = {false};
    bool snip;
    Node<T>* pred = nullptr;
    Node<T>* curr = nullptr;
    Node<T>* succ = nullptr;

retry:
    while (true) {
        pred = head;
        for (int level = maxLevel; level >= bottomLevel; level--) {
            curr = pred->next[level]->getReference();

            while (start >= curr->getEnd()) {
                succ = curr->next[level]->get(marked);
                while (marked[0]) {
                    snip = pred->next[level]->compareAndSet(curr, succ, false,
                                                            false);

                    if (!snip) goto retry;

                    curr = pred->next[level]->getReference();
                    succ = curr->next[level]->get(marked);
                }
                if (end >= curr->getStart()) {
                    pred = curr;
                    curr = succ;
                } else {
                    break;
                }
            }

            preds[level] = pred;
            succs[level] = curr;
        }
        return (end > curr->getStart());
    }
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::findExact(const T& start, const T& end,
                                                 Node<T>** preds,
                                                 Node<T>** succs) {
    int bottomLevel = 0;
    bool marked[1] = {false};
    bool snip;
    Node<T>* pred = nullptr;
    Node<T>* curr = nullptr;
    Node<T>* succ = nullptr;

retry:
    while (true) {
        pred = head;
        for (int level = maxLevel; level >= bottomLevel; level--) {
            curr = pred->next[level]->getReference();

            while (start >= curr->getStart()) {
                succ = curr->next[level]->get(marked);
                while (marked[0]) {
                    snip = pred->next[level]->compareAndSet(curr, succ, false,
                                                            false);

                    if (!snip) goto retry;

                    curr = pred->next[level]->getReference();
                    succ = curr->next[level]->get(marked);
                }
                if (start >= curr->getEnd()) {
                    pred = curr;
                    curr = succ;
                } else {
                    break;
                }
            }

            preds[level] = pred;
            succs[level] = curr;
        }
        return (start == curr->getStart() && end == curr->getEnd());
    }
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::tryLock(T start, T end) {
    int topLevel = randomLevel();
    int bottomLevel = 0;
    Node<T>* preds[maxLevel + 1];
    Node<T>* succs[maxLevel + 1];

    while (true) {
        bool found = find(start, end, preds, succs);
        if (found) {
            return false;
        } else {
            Node<T>* newNode = new Node<T>();
            newNode->initialize(start, end, topLevel);

            for (int level = bottomLevel; level <= topLevel; ++level) {
                Node<T>* succ = succs[level];
                newNode->next[level]->store(succ, false);
            }

            Node<T>* pred = preds[bottomLevel];
            Node<T>* succ = succs[bottomLevel];

            newNode->next[bottomLevel]->store(succ, false);
            if (!pred->next[bottomLevel]->compareAndSet(succ, newNode, false,
                                                        false)) {
                continue;
            }

            for (int level = bottomLevel + 1; level <= topLevel; ++level) {
                while (true) {
                    pred = preds[level];
                    succ = succs[level];
                    if (pred->next[level]->compareAndSet(succ, newNode, false,
                                                         false)) {
                        break;
                    }

                    find(start, end, preds, succs);
                }
            }
            elementsCount.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::releaseLock(T start, T end) {
    int bottomLevel = 0;
    Node<T>* preds[maxLevel + 1];
    Node<T>* succs[maxLevel + 1];
    Node<T>* succ;

    while (true) {
        bool found = findExact(start, end, preds, succs);
        if (!found) {
            std::cerr << "Range not found. Wrong ussage of releaseLock. "
                      << start << " " << end << std::endl;
            return false;
        } else {
            Node<T>* nodeToRemove = succs[bottomLevel];
            for (int level = nodeToRemove->getTopLevel();
                 level >= bottomLevel + 1; level--) {
                bool marked[1] = {false};
                succ = nodeToRemove->next[level]->get(marked);
                while (!marked[0]) {
                    nodeToRemove->next[level]->attemptMark(succ, true);
                    succ = nodeToRemove->next[level]->get(marked);
                }
            }

            bool marked[1] = {false};
            succ = nodeToRemove->next[bottomLevel]->get(marked);
            while (true) {
                bool iMarkedIt = nodeToRemove->next[bottomLevel]->compareAndSet(
                    succ, succ, false, true);
                succ = succs[bottomLevel]->next[bottomLevel]->get(marked);
                if (iMarkedIt) {
                    findExact(start, end, preds, succs);

                    elementsCount.fetch_sub(1, std::memory_order_relaxed);
                    return true;
                } else if (marked[0]) {
                    std::cerr << "Other thread is trying to release this "
                                 "range. Wrong usage of releaseLock somewhere"
                              << std::endl;
                    return false;
                }
            }
        }
    }
}

template <typename T, unsigned maxLevel>
void ConcurrentRangeLock<T, maxLevel>::displayList() {
    std::cout << "Concurrent Range Lock" << std::endl;

    if (this->elementsCount == 0) {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(this->elementsCount);

    std::vector<std::vector<std::string>> builder(
        len, std::vector<std::string>(maxLevel + 1));

    Node<T>* current = head->next[0]->getReference();

    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < maxLevel + 1; ++j) {
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
        current = current->next[0]->getReference();
    }

    for (int i = maxLevel; i >= 0; --i) {
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