#pragma once
#include <atomic>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#include "node.hpp"

template <typename T, unsigned maxLevel>
struct ConcurrentRangeLock
{
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
size_t ConcurrentRangeLock<T, maxLevel>::size()
{
    return this->elementsCount.load(std::memory_order_relaxed);
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock<T, maxLevel>::ConcurrentRangeLock()
{
    std::srand(std::time(0));

    auto min = std::numeric_limits<T>::min();
    auto max = std::numeric_limits<T>::max();

    head = createNode(min, min, maxLevel);
    tail = createNode(max, max, maxLevel);

    for (unsigned level = 0; level <= maxLevel; ++level)
    {
        head->next[level] = tail;
    }
}

template <typename T, unsigned maxLevel>
ConcurrentRangeLock<T, maxLevel>::~ConcurrentRangeLock()
{
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
unsigned ConcurrentRangeLock<T, maxLevel>::generateRandomLevel()
{
    float randNum = static_cast<float>(rand() / RAND_MAX);

    float threshold = 0.5; // Probability for level 1
    unsigned level = 1;

    while (randNum > threshold && level < maxLevel) {
        randNum -= threshold;
        threshold /= 2;
        level++;
    }

    return level;
}

template <typename T, unsigned maxLevel>
Node<T> *ConcurrentRangeLock<T, maxLevel>::createNode(T start, T end, int level)
{
    return new Node<T>(start, end, level);
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::findInsert(T start, T end, Node<T> **preds, Node<T> **succs)
{
    int levelFound = -1;
    Node<T> *pred = head;

    for (int level = maxLevel; level >= 0; level--)
    {
        Node<T> *curr = pred->next[level];

        while (start > curr->getEnd())
        {
            pred = curr;
            curr = pred->next[level];
        }

        if (levelFound == -1 && end >= curr->getStart())
        {
            levelFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return levelFound;
}

template <typename T, unsigned maxLevel>
int ConcurrentRangeLock<T, maxLevel>::findExact(T start, T end, Node<T> **preds, Node<T> **succs)
{
    int levelFound = -1;
    Node<T> *pred = head;

    for (int level = maxLevel; level >= 0; level--)
    {
        Node<T> *curr = pred->next[level];

        while (start > curr->getEnd())
        {
            pred = curr;
            curr = pred->next[level];
        }

        if (levelFound == -1 && start == curr->getStart() && end == curr->getEnd())
        {
            levelFound = level;
        }

        preds[level] = pred;
        succs[level] = curr;
    }

    return levelFound;
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::searchLock(T start, T end)
{
    Node<T> *preds[maxLevel + 1];
    Node<T> *succs[maxLevel + 1];

    int levelFound = findExact(start, end, preds, succs);

    return (levelFound != -1 && succs[levelFound]->fullyLinked &&
            !succs[levelFound]->marked);
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::tryLock(T start, T end)
{
    int topLevel = static_cast<int>(generateRandomLevel());
    Node<T> *preds[maxLevel + 1];
    Node<T> *succs[maxLevel + 1];

    while (true)
    {
        int levelFound = findInsert(start, end, preds, succs);
        if (levelFound != -1)
        {
            Node<T> *nodeFound = succs[levelFound];
            if (!nodeFound->marked)
            {
                while (!nodeFound->fullyLinked)
                {
                    std::this_thread::yield();
                }
                return false;
            }
            continue;
        }

        int highestLocked = -1;
        bool valid = true;

        for (int level = 0; valid && level <= topLevel; level++)
        {
            Node<T> *pred = preds[level];
            Node<T> *succ = succs[level];
            pred->lock();
            highestLocked = level;
            valid = !pred->marked && !succ->marked && pred->next[level] == succ;
        }

        ScopeGuard unlockGuard([&preds, highestLocked]()
                               {
                for (int level = highestLocked; level >= 0; level--)
                {
                    preds[level]->unlock();
                } });

        if (!valid)
            continue;

        Node<T> *newNode = createNode(start, end, topLevel);
        for (int level = 0; level <= topLevel; level++)
        {
            newNode->next[level] = succs[level];
            preds[level]->next[level] = newNode;
        }
        newNode->fullyLinked = true;


        currentLevel = topLevel  > currentLevel ? topLevel : currentLevel;
        elementsCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
}

template <typename T, unsigned maxLevel>
bool ConcurrentRangeLock<T, maxLevel>::releaseLock(T start, T end)
{
    Node<T> *victim = nullptr;
    bool isMarked = false;
    int topLevel = -1;

    Node<T> *preds[maxLevel + 1];
    Node<T> *succs[maxLevel + 1];

    while (true)
    {
        int levelFound = findExact(start, end, preds, succs);
        if (levelFound != -1)
        {
            victim = succs[levelFound];
        }

        if (isMarked || (levelFound != -1 && victim->getTopLevel() == levelFound && !victim->marked))
        {
            if (!isMarked)
            {
                topLevel = victim->getTopLevel();
                victim->lock();
                if (victim->marked)
                {
                    victim->unlock();
                    return false;
                }
                victim->marked = true;
                isMarked = true;
            }

            int highestLocked = -1;
            bool valid = true;
            Node<T> *pred, *succ;

            for (int level = 0; valid && level <= topLevel; level++)
            {
                pred = preds[level];
                pred->lock();
                highestLocked = level;
                valid = !pred->marked && pred->next[level] == victim;
            }

            ScopeGuard unlockGuard([&preds, highestLocked]()
                                   {
                for (int level = highestLocked; level >= 0; level--)
                {
                    preds[level]->unlock();
                } });

            if (!valid)
                continue;

            for (int level = topLevel; level >= 0; level--)
            {
                preds[level]->next[level] = victim->next[level];
            }
            victim->unlock();

            elementsCount.fetch_sub(1, std::memory_order_relaxed);

            return true;
        }
        else
        {
            return false;
        }
    }
}

template <typename T, unsigned maxLevel>
void ConcurrentRangeLock<T, maxLevel>::displayList()
{
    std::cout << "Concurrent Range Lock" << std::endl;

    if (head->next[0] == nullptr)
    {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(this->elementsCount);

    std::vector<std::vector<std::string>> builder(len, std::vector<std::string>(this->currentLevel + 1));

    Node<T> *current = head->next[0];

    for (int i = 0; i < len; ++i)
    {
        for (int j = 0; j < this->currentLevel + 1; ++j)
        {
            if (j < current->getTopLevel() + 1)
            {
                std::ostringstream oss;
                oss << "[" << std::setw(2) << std::setfill('0') << current->getStart() << ","
                    << std::setw(2) << std::setfill('0') << current->getEnd() << "]";
                builder[i][j] = oss.str();
            }
            else
            {
                builder[i][j] = "---------";
            }
        }
        current = current->next[0];
    }

    for (int i = this->currentLevel; i >= 0; --i)
    {
        std::cout << "Level " << i << ": head ";
        for (int j = 0; j < len; ++j)
        {
            if (builder[j][i] == "---------")
            {
                std::cout << "---------";
            }
            else
            {
                std::cout << "->" << builder[j][i];
            }
        }
        std::cout << "---> tail" << std::endl;
    }
}
