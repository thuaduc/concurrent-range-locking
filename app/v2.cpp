#include <iostream>

#include "../src/v1/range_lock.cpp"
#include "../src/v2/range_lock.hpp"

int main() {
    ConcurrentRangeLock<uint64_t, 5> list = ConcurrentRangeLock<uint64_t, 5>();

    list.tryLock(5, 10);
    list.tryLock(20, 30);
    list.tryLock(11, 12);
    list.tryLock(30, 40);
    list.tryLock(1, 5);
    list.displayList();

    list.releaseLock(5, 10);
    list.tryLock(40, 50);
    list.tryLock(60, 70);

    list.displayList();

    // ListRL list;
    // MutexRangeAcquire(&list, 0, 10);
    // MutexRangeAcquire(&list, 11, 20);
    // MutexRangeAcquire(&list, 21, 30);
    // MutexRangeAcquire(&list, 31, 40);
    // MutexRangeAcquire(&list, 41, 50);
    // MutexRangeAcquire(&list, 51, 60);
    // MutexRangeAcquire(&list, 61, 70);

    // printList(&list);

    return 0;
}