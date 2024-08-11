#include <gtest/gtest.h>

#include <random>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../../src/v3/range_lock.hpp"

// Test case for concurrent insertions
TEST(XiangSongRangeLock, ConcurrentInsertions) {
    int num_threads = 1;
    int num_elements_per_thread = 10;
    SongRangeLock rl;

    auto tryLockFunc = [&](int thread_id) {
        for (int i = 0; i < num_elements_per_thread; i += 2) {
            int value = thread_id * num_elements_per_thread + i;
            ASSERT_TRUE(rl.tryLock(value, value + 1));
        }
        std::cout << std::endl;
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(tryLockFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    rl.displayList();

    ASSERT_EQ(rl.size(), num_threads * num_elements_per_thread / 2);
}

// Test case for concurrent deletions
TEST(XiangSongRangeLock, ConcurrentDeletions) {
    int num_elements = 10000;
    SongRangeLock rl;

    for (int i = 0; i < num_elements; i += 2) {
        rl.tryLock(i, i + 1);
    }

    ASSERT_EQ(rl.size(), 5000);

    auto releaseLockFunc = [&](int start, int end) {
        for (int i = start; i < end; i += 2) {
            rl.releaseLock(i);
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

    ASSERT_EQ(rl.size(), 0);
}

// Test case for all operations concurrently
TEST(XiangSongRangeLock, MixedOperationsConcurrently) {
    const int num_threads = 50;
    const int num_operations_per_thread = 1000;
    SongRangeLock rl;

    auto mixedOpFunc = [&](int thread_id) {
        for (int i = 0; i < num_operations_per_thread; i += 2) {
            int value = thread_id * num_operations_per_thread + i;

            rl.tryLock(value, value + 1);
            rl.releaseLock(value);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(rl.size(), 0);
}

// Test case for correct deletion of unordered insertion
TEST(XiangSongRangeLock, CorrectInsertion) {
    const int num_threads = 2;
    SongRangeLock rl;

    std::vector<std::pair<int, int>> ranges;
    std::vector<std::pair<int, int>> rangesShuffled;

    for (int i = 0; i < 1000; i += 10) {
        ranges.push_back(std::make_pair(i, i + 5));
        rangesShuffled.push_back(std::make_pair(i, i + 5));
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(rangesShuffled.begin(), rangesShuffled.end(),
                 std::default_random_engine(seed));

    auto mixedOpFunc = [&]() {
        for (int i = 0; i < rangesShuffled.size(); ++i) {
            auto start = rangesShuffled[i].first;
            auto end = rangesShuffled[i].second;

            auto res = rl.tryLock(start, end);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(rl.size(), ranges.size());

    for (int i = 0; i < ranges.size(); i++) {
        auto start = ranges[i].first;
        auto end = ranges[i].second;

        rl.releaseLock(start);
    }

    ASSERT_EQ(rl.size(), 0);
}

// Test case for correct order of insertion
TEST(XiangSongRangeLock, CorrectOrderOfInsertion) {
    std::unordered_map<int, std::string> database;
    std::uniform_int_distribution<int> dist(0, 10000);
    std::uniform_int_distribution<int> range_dist(10, 100);

    const int num_threads = 50;
    const int num_transactions = 1000;
    SongRangeLock rl;

    auto mixedOpFunc = [&](int thread_id) {
        std::mt19937 rng(thread_id);

        for (int i = 0; i < num_transactions; ++i) {
            int start = dist(rng);
            int end = start + range_dist(rng);

            rl.tryLock(start, end);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto pred = rl.head_;
    for (auto curr = pred->forward[0]; curr != rl.tail_;) {
        ASSERT_TRUE(pred->end <= curr->start);
        ASSERT_TRUE(pred->start <= curr->start);
        pred = curr;
        curr = pred->forward[0];
    }
}