#include <cassert>
#include <iostream>
#include <thread>

#include "../src/v4/concurrent_tree.h"
#include "../src/v4/keyrange.h"

int main() {
    concurrent_tree::locked_keyrange lkr;

    keyrange range;
    range.create(1, 2);

    concurrent_tree* tree = new concurrent_tree();
    tree->create();

    lkr.prepare(tree);
    lkr.acquire(range);
}

// int main() {
//     const int num_threads = 2;
//     RangeLock rl;

//     std::vector<std::pair<int, int>> ranges;
//     std::vector<std::pair<int, int>> rangesShuffled;

//     for (int i = 0; i < 1000; i += 10) {
//         ranges.push_back(std::make_pair(i, i + 5));
//         rangesShuffled.push_back(std::make_pair(i, i + 5));
//     }
//     unsigned seed =
//     std::chrono::system_clock::now().time_since_epoch().count();
//     std::shuffle(rangesShuffled.begin(), rangesShuffled.end(),
//                  std::default_random_engine(seed));

//     auto mixedOpFunc = [&]() {
//         for (int i = 0; i < rangesShuffled.size(); ++i) {
//             auto start = rangesShuffled[i].first;
//             auto end = rangesShuffled[i].second;

//             rl.tryLock(start, end);
//         }
//     };

//     std::vector<std::thread> threads;
//     for (int i = 0; i < num_threads; ++i) {
//         threads.emplace_back(mixedOpFunc);
//     }

//     for (auto& t : threads) {
//         t.join();
//     }

//     std::cout << "size: " << rl.size() << std::endl;
//     std::cout << ranges.size() << " " << rangesShuffled.size() << std::endl;

//     for (int i = 0; i < ranges.size(); i++) {
//         auto start = ranges[i].first;
//         auto end = ranges[i].second;

//         rl.releaseLock(start);
//     }

//     std::cout << "size: " << rl.size() << std::endl;
// }

// int main() {
// SongRangeLock rl;
// rl.tryLock(0, 1);
// rl.tryLock(3, 4);
// rl.tryLock(3, 4);
// rl.tryLock(3, 4);
// rl.tryLock(4, 5);
// rl.tryLock(5, 6);

// rl.displayList();

// ConcurrentRangeLock<uint64_t, 4> crl;
// crl.tryLock(3, 4);
// crl.tryLock(3, 4);
// crl.tryLock(3, 4);
// crl.tryLock(4, 5);
// crl.tryLock(5, 6);

// crl.displayList();

// RangeLock rl;

// for (int i = 0; i < 100; i += 2) {
//     rl.tryLock(i, i + 1);
// }

// for (int i = 0; i < 90; i += 2) {
//     rl.releaseLock(i);
// }

// rl.displayList();

// std::cout << "size " << rl.size() << std::endl;

// ConcurrentRangeLock<uint64_t, 4> list = ConcurrentRangeLock<uint64_t,
// 4>();

// list.tryLock(20, 30);
// list.tryLock(320, 330);
// list.tryLock(40, 50);
// list.tryLock(5, 10);
// list.tryLock(80, 100);
// list.tryLock(140, 150);
// list.tryLock(304, 305);
// list.tryLock(160, 240);
// list.tryLock(302, 303);
// list.tryLock(300, 301);
// list.tryLock(130, 140);
// list.tryLock(305, 310);
// list.tryLock(110, 120);

// list.displayList();

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

// return 0;
// }