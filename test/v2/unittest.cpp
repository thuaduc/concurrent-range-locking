#include <gtest/gtest.h>

#include <set>
#include <thread>
#include <vector>

#include "../../src/v2/range_lock.hpp"

// Predefined maxLevel
constexpr unsigned maxLevel = 16;

// Test case for concurrent insertions
TEST(ConcurrentRangeLock, ConcurrentInsertions) {
    int num_threads = 10;
    int num_elements_per_thread = 100;
    ConcurrentRangeLock<int, maxLevel> crl{};

    auto tryLockFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; i += 2) {
            int value = thread_id * num_elements_per_thread + i;

            ASSERT_TRUE(crl.tryLock(value, value + 1));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(tryLockFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), num_threads * num_elements_per_thread / 2);
}

// Test case for concurrent deletions
TEST(ConcurrentRangeLock, ConcurrentDeletions) {
    int num_elements = 1000;
    ConcurrentRangeLock<int, maxLevel> crl{};

    for (int i = 0; i < num_elements; i += 2) {
        crl.tryLock(i, i + 1);
    }

    auto releaseLockFunc = [&](int start, int end) {
        for (int i = start; i < end; i += 2) {
            auto res = crl.releaseLock(i, i + 1);
            ASSERT_TRUE(res);
        }
    };

    std::vector<std::thread> threads;

    threads.emplace_back(releaseLockFunc, 0, 500);
    threads.emplace_back(releaseLockFunc, 500, num_elements);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), 0);
}

// Test case for concurrent searches
TEST(ConcurrentRangeLock, ConcurrentSearches) {
    int num_elements = 100;
    ConcurrentRangeLock<int, maxLevel> crl{};

    for (int i = 0; i < num_elements; i += 2) {
        crl.tryLock(i, i + 1);
    }

    const auto searchFunc = [&](int start, int end) {
        for (int i = start; i < end; i += 2) {
            // ASSERT_TRUE(crl.searchLock(i, i + 1));
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(searchFunc, 0, num_elements / 2);
    threads.emplace_back(searchFunc, num_elements / 2, num_elements);

    for (auto& t : threads) {
        t.join();
    }
}

// Test case for all operations concurrently
TEST(ConcurrentRangeLock, MixedOperationsConcurrently) {
    const int num_threads = 50;
    const int num_operations_per_thread = 1000;
    ConcurrentRangeLock<int, maxLevel> crl{};

    auto mixedOpFunc = [&](int thread_id) {
        for (int i = 0; i < num_operations_per_thread; i += 2) {
            int value = thread_id * num_operations_per_thread + i;

            // Even thread IDs insert, odd thread IDs search
            // All threads attempt to delete
            if (thread_id % 2 == 0) {
                crl.tryLock(value, value + 1);
            } else {
                // crl.searchLock(value, value + 1);
            }
            crl.releaseLock(value, value + 1);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), 0);
}

// Test with high number of threads performing insertions
TEST(ConcurrentRangeLock, HighConcurrencyInsertions) {
    const int num_threads = 50;
    const int num_elements_per_thread = 20;
    ConcurrentRangeLock<int, maxLevel> crl{};

    auto tryLockFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; i += 2) {
            int value = thread_id * num_elements_per_thread + i;
            crl.tryLock(value, value + 1);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(tryLockFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), num_threads * num_elements_per_thread / 2);
}

// Test case for validating list integrity after concurrent deletions
TEST(ConcurrentRangeLock, ValidateIntegrityAfterConcurrentDeletions) {
    const int num_elements = 1000;
    ConcurrentRangeLock<int, maxLevel> crl{};

    for (int i = 0; i < num_elements; i += 2) {
        crl.tryLock(i, i + 1);
    }

    crl.displayList();

    auto releaseLockFunc = [&](int start, int end) {
        for (int i = start; i < end; i += 2) {
            crl.releaseLock(i, i + 1);
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(releaseLockFunc, 0, 500);
    threads.emplace_back(releaseLockFunc, 500, num_elements);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), 0);
}

// Edge case test: consecutive insertions and deletions same elements
TEST(ConcurrentRangeLock, RapidConsecutiveInsertionsAndDeletions) {
    const int num_threads = 10;
    const int value = 123;
    ConcurrentRangeLock<int, maxLevel> crl{};

    auto insertDeleteFunc = [&](int) {
        for (int i = 0; i < 100; i += 2) {
            crl.tryLock(value, value + 1);
            crl.releaseLock(value, value + 1);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(insertDeleteFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Simple test from leanstore
// TEST(ConcurrentRangeLock, Simple) {
//     int NO_THREADS = 50;

//     worker_thread_id = 0;

//     ConcurrentRangeLock<int, maxLevel> crl{};

//     EXPECT_TRUE(crl.tryLock(101, 50));
//     EXPECT_FALSE(crl.tryLock(100, 2));
//     EXPECT_TRUE(crl.tryLock(100, 1));
//     EXPECT_FALSE(crl.tryLock(50, 100));
//     EXPECT_FALSE(crl.tryLock(50, 200));
//     EXPECT_FALSE(crl.tryLock(150, 50));

//     EXPECT_FALSE(crl.searchLock(120, 10));
//     EXPECT_FALSE(crl.searchLock(100, 1));
//     EXPECT_FALSE(crl.searchLock(90, 20));
//     EXPECT_TRUE(crl.searchLock(101, 10));
// }

// TEST(ConcurrentRangeLock, Concurrency) {
//     ConcurrentRangeLock<int, maxLevel> crl{};

//     std::thread threads[NO_THREADS];

//     for (int idx = 0; idx < NO_THREADS; idx++) {
//         threads[idx] = std::thread([&, t_id = idx]() {
//             worker_thread_id = t_id;

//             for (auto i = 0; i < 10000; i++) {
//                 EXPECT_TRUE(crl.tryLock(t_id * 100 + 1, 100));
//                 EXPECT_FALSE(crl.searchLock(t_id * 100 + 1, 100));
//                 crl.unlockRange(t_id * 100 + 1, 100);
//             }
//         });
//     }

//     for (auto& thread : threads) {
//         thread.join();
//     }
// }
