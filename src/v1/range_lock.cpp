#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

// Node structure for the linked list
struct LNode {
    uint64_t start;
    uint64_t end;
    std::atomic<LNode*> next;

    LNode(uint64_t s, uint64_t e) : start(s), end(e), next(nullptr) {}
};

// List structure for range locks
struct ListRL {
    std::atomic<LNode*> head;

    ListRL() : head(nullptr) {}
};

// Range lock structure
struct RangeLock {
    LNode* node;

    RangeLock(LNode* n) : node(n) {}
};

// Check if node is marked
bool isMarked(LNode* node) {
    return (reinterpret_cast<uintptr_t>(node) & 1) != 0;
}

// Unmark the node
LNode* unmark(LNode* node) {
    return reinterpret_cast<LNode*>(reinterpret_cast<uintptr_t>(node) & ~1);
}

// Compare the range of two node
int compare(LNode* lock1, LNode* lock2) {
    if (!lock1) return 1;  // lock1 is end of the list, no overlap
    if (lock1->start >= lock2->end)
        return 1;  // lock1 comes after lock2, no overlap
    if (lock2->start >= lock1->end)
        return -1;  // lock1 is before lock2, no overlap
    return 0;       // lock1 and lock2 overlap
}

// Insert node into the list
bool InsertNode(ListRL* listrl, LNode* lock) {
    while (true) {
        std::atomic<LNode*>* prev = &(listrl->head);
        LNode* cur = prev->load();
        while (true) {
            if (isMarked(cur)) break;  // prev is logically deleted

            if (cur &&
                isMarked(cur->next.load())) {  // cur is logically deleted
                LNode* next = unmark(cur->next.load());
                std::atomic_compare_exchange_strong(
                    prev, &cur, next);  // try to remove it from list
                cur = next;
            } else {  // cur is currently protecting a range
                int ret = compare(cur, lock);
                if (ret == -1) {  // lock succeeds cur
                    prev = &(cur->next);
                    cur = prev->load();
                } else if (ret == 0) {  // lock overlaps with cur
                    return false;
                } else if (ret ==
                           1) {  // lock precedes cur or reached end of list
                    lock->next.store(cur);
                    if (std::atomic_compare_exchange_strong(prev, &cur, lock)) {
                        return true;  // success - the range is acquired now
                    }
                    cur =
                        prev->load();  // otherwise continue traversing the list
                }
            }
        }
    }
}

// Delete node from the list
void DeleteNode(LNode* lock) {
    LNode* currentNext = lock->next.load();
    LNode* markedNext =
        reinterpret_cast<LNode*>(reinterpret_cast<uintptr_t>(currentNext) | 1);
    lock->next.store(markedNext);
}

// Acquire a range lock
RangeLock* MutexRangeAcquire(ListRL* listrl, uint64_t start, uint64_t end) {
    RangeLock* rl = new RangeLock(new LNode(start, end));
    if (InsertNode(listrl, rl->node)) {
        return rl;
    }
    delete rl;
    return nullptr;
}

// Release a range lock
void MutexRangeRelease(RangeLock* rl) { DeleteNode(rl->node); }

// Print the range lock
void printList(ListRL* listrl) {
    LNode* cur = listrl->head.load();
    while (cur) {
        if (isMarked(cur)) {
            std::cout << "[X] ";
        } else {
            std::cout << "[" << cur->start << ", " << cur->end << "] ";
        }
        cur = cur->next.load();
    }
    std::cout << std::endl;
}