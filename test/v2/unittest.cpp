#include <gtest/gtest.h>

#include <set>
#include <thread>
#include <vector>
#include <unordered_map>
#include <random>

#include "../../src/v2/range_lock.hpp"

// Predefined maxLevel
constexpr unsigned maxLevel = 16;

// Test case for concurrent insertions
TEST(ConcurrentRangeLock, ConcurrentInsertions) {
    int num_threads = 50;
    int num_elements_per_thread = 1000;
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
    int num_elements = 10000;
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

    threads.emplace_back(releaseLockFunc, 0, 2000);
    threads.emplace_back(releaseLockFunc, 2000, 4000);
    threads.emplace_back(releaseLockFunc, 4000, 6000);
    threads.emplace_back(releaseLockFunc, 6000, 8000);
    threads.emplace_back(releaseLockFunc, 8000, 10000);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), 0);
}

// Test case for all operations concurrently
TEST(ConcurrentRangeLock, MixedOperationsConcurrently) {
    const int num_threads = 50;
    const int num_operations_per_thread = 1000;
    ConcurrentRangeLock<int, maxLevel> crl{};

    auto mixedOpFunc = [&](int thread_id) {
        for (int i = 0; i < num_operations_per_thread; i += 2) {
            int value = thread_id * num_operations_per_thread + i;

            crl.tryLock(value, value + 1);
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

// Test case for correct order of insertion
TEST(ConcurrentRangeLock, CorrectInsertion) {
    const int num_threads = 50;
    ConcurrentRangeLock<int, maxLevel> crl{};

    std::vector<std::pair<int, int>> ranges;
    std::vector<std::pair<int, int>> rangesShuffled;

    for (int i = 0; i < 1000; i += 10) {
        ranges.push_back(std::make_pair(i, i + 9));
        rangesShuffled.push_back(std::make_pair(i, i + 9));
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(rangesShuffled.begin(), rangesShuffled.end(),
            std::default_random_engine(seed));


    auto mixedOpFunc = [&]() {
        for (int i = 0; i < rangesShuffled.size(); ++i) {
            auto start = rangesShuffled[i].first;
            auto end = rangesShuffled[i].second;

            crl.tryLock(start, end);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(crl.size(), ranges.size());

    for (int i = 0; i < ranges.size(); i ++) {
        auto start = ranges[i].first;
        auto end = ranges[i].second;

        crl.releaseLock(start, end);
    }

    ASSERT_EQ(crl.size(), 0);
}

// Test case for correct order of insertion
TEST(ConcurrentRangeLock, CorrectOrderOfInsertion) {
    std::unordered_map<int, std::string> database;
    std::uniform_int_distribution<int> dist(0, 9'999'00);
    std::uniform_int_distribution<int> range_dist(10, 1000);

    const int num_threads = 50;
    const int num_transactions = 5000;
    ConcurrentRangeLock<int, maxLevel> crl{};

    auto mixedOpFunc = [&](int thread_id){
        std::mt19937 rng(thread_id);

        for (int i = 0; i < num_transactions; ++i) {
            int start = dist(rng);
            int end = start + range_dist(rng);

            crl.tryLock(start, end);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // crl.displayList();

    auto pred = crl.head;
    for (auto curr = pred->next[0]->getReference(); curr != crl.tail;) {
        ASSERT_TRUE(pred->getEnd() < curr->getStart());
        ASSERT_TRUE(pred->getStart() < curr->getStart());
        pred = curr;
        curr = pred->next[0]->getReference();
    }

}