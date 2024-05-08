#include <gtest/gtest.h>

#include <set>
#include <thread>
#include <vector>

#include "concurrentSkipList.hpp"

// Predefined maxLevel
constexpr unsigned maxLevel = 16;

// Test case for concurrent insertions
TEST(ConcurrentRangeLock, ConcurrentInsertions) {
    int num_threads = 10;
    int num_elements_per_thread = 100;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    auto insertFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; ++i) {
            int value = thread_id * num_elements_per_thread + i;

            ASSERT_TRUE(csl.insertElement(value, value));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(insertFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(csl.size(), num_threads * num_elements_per_thread);
}

// Test case for concurrent deletions
TEST(ConcurrentRangeLock, ConcurrentDeletions) {
    int num_elements = 1000;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    for (int i = 0; i < num_elements; ++i) {
        csl.insertElement(i, i);
    }

    auto deleteFunc = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            const auto res = csl.deleteElement(i);
            ASSERT_TRUE(res);
        }
    };

    std::vector<std::thread> threads;

    threads.emplace_back(deleteFunc, 0, 500);
    threads.emplace_back(deleteFunc, 500, num_elements);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(csl.size(), 0);
}

// Test case for concurrent searches
TEST(ConcurrentRangeLock, ConcurrentSearches) {
    int num_elements = 100;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    for (int i = 0; i < num_elements; ++i) {
        csl.insertElement(i, i);
    }

    const auto searchFunc = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            ASSERT_TRUE(csl.searchElement(i));
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
    const int num_threads = 20;
    const int num_operations_per_thread = 50;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    auto mixedOpFunc = [&](int thread_id) {
        for (int i = 0; i < num_operations_per_thread; ++i) {
            int value = thread_id * num_operations_per_thread + i;

            // Even thread IDs insert, odd thread IDs search
            // All threads attempt to delete
            if (thread_id % 2 == 0) {
                csl.insertElement(value, value);
            } else {
                csl.searchElement(value);
            }
            csl.deleteElement(value);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(csl.size(), 0);
}

// Test with high number of threads performing insertions
TEST(ConcurrentRangeLock, HighConcurrencyInsertions) {
    const int num_threads = 50;
    const int num_elements_per_thread = 20;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    auto insertFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; ++i) {
            int value = thread_id * num_elements_per_thread + i;
            csl.insertElement(value, value);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(insertFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(csl.size(), num_threads * num_elements_per_thread);
}

// Test case for validating list integrity after concurrent deletions
TEST(ConcurrentRangeLock, ValidateIntegrityAfterConcurrentDeletions) {
    const int num_elements = 1000;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    for (int i = 0; i < num_elements; ++i) {
        csl.insertElement(i, i);
    }

    auto deleteFunc = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            csl.deleteElement(i);
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(deleteFunc, 0, 500);
    threads.emplace_back(deleteFunc, 500, num_elements);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(csl.size(), 0);
}

// Edge case test: consecutive insertions and deletions same elements
TEST(ConcurrentRangeLock, RapidConsecutiveInsertionsAndDeletions) {
    const int num_threads = 10;
    const int value = 123;
    ConcurrentRangeLock<int, int, maxLevel> csl{};

    auto insertDeleteFunc = [&](int) {
        for (int i = 0; i < 100; ++i) {
            csl.insertElement(value, value);
            csl.deleteElement(value);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(insertDeleteFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // The final state should not contain the repeatedly inserted-deleted
    // element
    ASSERT_FALSE(csl.searchElement(value));
}
