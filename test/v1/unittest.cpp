#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "../../src/v1/range_lock.cpp"

// Test case for concurrent insertions
TEST(ConcurrentRangeLock, ConcurrentInsertions) {
    ListRL myList;
    const int num_threads = 10;
    const int num_elements_per_thread = 100;

    auto tryLockFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; i += 2) {
            uint64_t value = thread_id * num_elements_per_thread + i;
            auto rl = MutexRangeAcquire(&myList, value, value + 1);
            ASSERT_NE(rl, nullptr);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(tryLockFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Test case for concurrent deletions
TEST(ConcurrentRangeLock, ConcurrentDeletions) {
    ListRL myList;
    const int num_elements = 1000;

    std::vector<RangeLock*> locks;
    for (int i = 0; i < num_elements; i += 2) {
        locks.push_back(MutexRangeAcquire(&myList, i, i + 1));
    }

    auto releaseLockFunc = [&](int start, int end) {
        for (int i = start; i < end; i += 2) {
            MutexRangeRelease(&myList,locks[i / 2]);
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(releaseLockFunc, 0, 500);
    threads.emplace_back(releaseLockFunc, 500, num_elements);

    for (auto& t : threads) {
        t.join();
    }
}

// Test case for all operations concurrently
TEST(ConcurrentRangeLock, MixedOperationsConcurrently) {
    ListRL myList;
    const int num_threads = 20;
    const int num_operations_per_thread = 1000;

    auto mixedOpFunc = [&](int thread_id) {
        for (int i = 0; i < num_operations_per_thread; i += 2) {
            uint64_t value = thread_id * num_operations_per_thread + i;

            if (thread_id % 2 == 0) {
                MutexRangeAcquire(&myList, value, value + 1);
            } else {
                ASSERT_NE(MutexRangeAcquire(&myList, value, value + 1),
                          nullptr);
            }
            MutexRangeRelease(&myList,new RangeLock(new LNode(value, value + 1)));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Test with high number of threads performing insertions
TEST(ConcurrentRangeLock, HighConcurrencyInsertions) {
    ListRL myList;
    const int num_threads = 50;
    const int num_elements_per_thread = 20;

    auto tryLockFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; i += 2) {
            uint64_t value = thread_id * num_elements_per_thread + i;
            MutexRangeAcquire(&myList, value, value + 1);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(tryLockFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Test case for validating list integrity after concurrent deletions
TEST(ConcurrentRangeLock, ValidateIntegrityAfterConcurrentDeletions) {
    ListRL myList;
    const int num_elements = 1000;

    std::vector<RangeLock*> locks;
    for (int i = 0; i < num_elements; i += 2) {
        locks.push_back(MutexRangeAcquire(&myList, i, i + 1));
    }

    auto releaseLockFunc = [&](int start, int end) {
        for (int i = start; i < end; i += 2) {
            MutexRangeRelease(&myList, locks[i / 2]);
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(releaseLockFunc, 0, 500);
    threads.emplace_back(releaseLockFunc, 500, num_elements);

    for (auto& t : threads) {
        t.join();
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}