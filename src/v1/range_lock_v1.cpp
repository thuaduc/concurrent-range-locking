#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

// Defines a node within the list
class LNode {
   public:
    uint64_t start;
    uint64_t end;
    std::shared_ptr<LNode> next;
    bool isMarked;

    LNode(uint64_t s, uint64_t e)
        : start(s), end(e), next(nullptr), isMarked(false) {}
};

// Defines a list for range locks protecting the same resource
class ListRL {
   public:
    std::shared_ptr<LNode> head;

    ListRL() : head(nullptr) {}
};

// Defines a range lock to protect a region within a shared resource
class RangeLock {
   public:
    std::shared_ptr<LNode> node;

    RangeLock() : node(nullptr) {}
};

int compare(const std::shared_ptr<LNode>& lock1,
            const std::shared_ptr<LNode>& lock2) {
    // Lock1 is end of the list, no overlap
    if (!lock1) return 1;

    // Check if lock1 comes after lock2, no overlap
    if (lock1->start >= lock2->end) return 1;

    // Check if lock1 is before lock2, no overlap
    if (lock2->start >= lock1->end) return -1;

    // Lock1 and Lock2 overlap
    return 0;
}

void InsertNode(ListRL* listrl, const std::shared_ptr<LNode>& lock) {
    while (true) {
        auto prev = listrl->head;
        auto cur = prev;
        while (true) {
            // Case prev is logically deleted
            if (prev && prev->isMarked)
                break;  // Traversal must restart as pointer to previous is lost

            // Case cur is logically deleted
            else if (cur && cur->isMarked) {
                auto next = cur->next;
                if (std::atomic_compare_exchange_strong(&prev, &cur, next)) {
                    cur = next;
                } else {
                    break;
                }

                // Case cur is currently protecting a range
            } else {
                auto ret = compare(cur, lock);

                // Lock succeeds cur
                if (ret == -1) {  // Continue traversing the list
                    prev = cur->next;
                    cur = prev;
                }

                // Lock overlap with cur
                else if (ret == 0) {
                    // std::cout << "Thread with id " <<
                    // std::this_thread::get_id()
                    //           << " waiting for node to be deleted "
                    //           << std::endl;

                    // Wait until cur marks itself as deleted
                    while (cur && !cur->isMarked) {
                        std::this_thread::yield();
                    }

                    // Lock preceeds cur or reached the end of list
                } else if (ret == 1) {
                    lock->next = cur;
                    // Insert lock into the list
                    if (std::atomic_compare_exchange_strong(&prev,&cur, lock))
                        return;   // Success -> the range is acquired now
                    cur = prev;  // Continue traversing the list
                }
            }
        }
    }
}

std::shared_ptr<RangeLock> MutexRangeAcquire(ListRL* listrl, uint64_t start,
                                             uint64_t end) {
    auto rl = std::make_shared<RangeLock>();
    rl->node = std::make_shared<LNode>(start, end);
    InsertNode(listrl, rl->node);
    return rl;
}

void MutexRangeRelease(const std::shared_ptr<RangeLock>& rl) {
    rl->node->isMarked == true;
}
