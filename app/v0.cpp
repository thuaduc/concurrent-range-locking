#include <iostream>

#include "../src/v1/range_lock.cpp"
#include "../src/v2/range_lock.hpp"

int main() {
    ConcurrentRangeLock<uint64_t, 4> list = ConcurrentRangeLock<uint64_t, 4>();

    list.tryLock(20, 30);
    list.tryLock(320, 330);
    list.tryLock(40, 50);
    list.tryLock(5, 10);
    list.tryLock(80, 100);
    list.tryLock(140, 150);
    list.tryLock(304, 305);
    list.tryLock(160, 240);
    list.tryLock(302, 303);
    list.tryLock(300, 301);
    list.tryLock(130, 140);
    list.tryLock(305, 310);
    list.tryLock(110, 120);

    list.displayList();

    /* auto mixedOpFunc = [&](uint64_t start, uint64_t end) {
         for (int i = 0; i < 1000; ++i) {
             list.tryLock(start, end);
         }
     };

     std::vector<std::thread> threads;
     for (int i = 0; i < 50; ++i) {
         threads.emplace_back(mixedOpFunc, 205925, 206889);
     }
     for (int i = 0; i < 50; ++i) {
         threads.emplace_back(mixedOpFunc, 205706, 206130);
     }

     for (auto &t: threads) {
         t.join();
     }

     list.displayList();*/

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