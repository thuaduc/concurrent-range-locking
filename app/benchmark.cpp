#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "../src/v0/range_lock.hpp"
#include "../src/v1/range_lock.cpp"

constexpr uint64_t numThreads = 50;
constexpr uint64_t range = 10000;
constexpr uint16_t lockHeight = 4;
constexpr int runtime = 10;

std::vector<std::pair<int, int>> createRanges(int x) {
    std::vector<std::pair<int, int>> ranges;
    for (int i = 0; i < x; i += 5) {
        ranges.push_back(std::make_pair(i, i + 8));
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(ranges.begin(), ranges.end(),
                 std::default_random_engine(seed));

    return ranges;
}

double thread_v0(std::vector<std::pair<int, int>> &ranges) {
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, &ranges, i] {
            for (size_t j = i; j < ranges.size(); j += numThreads) {
                auto start = ranges[j].first;
                auto end = ranges[j].second;

                bool res = crl.tryLock(start, end);
                // while (res != true) {
                //     res = crl.tryLock(start, end);
                // }

                // std::this_thread::sleep_for(
                //     std::chrono::microseconds(((start + end) * 100) % 1000));

                // crl.releaseLock(start, end);
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread : threads) {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;

    return duration.count();
}

double thread_v1(std::vector<std::pair<int, int>> &ranges) {
    ListRL list;
    std::mutex printMutex;

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&list, &ranges, i, &printMutex] {
            for (size_t j = i; j < ranges.size(); j += numThreads) {
                auto start = ranges[j].first;
                auto end = ranges[j].second;

                auto rl = MutexRangeAcquire(&list, start, end);
                // while (rl == nullptr) {
                //     rl = MutexRangeAcquire(&list, start, end);
                // }

                // std::this_thread::sleep_for(
                //     std::chrono::microseconds(((start + end) * 100) % 1000));

                // MutexRangeRelease(rl);
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread : threads) {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;

    return duration.count();
}

int main() {
    double v0 = 0;
    double v1 = 0;
    std::vector<std::pair<int, int>> ranges = createRanges(range);

    std::cout << "Benchmarking" << std::endl;

    for (int i = 0; i < runtime; i++) {
        v0 += thread_v0(ranges);
    }
    std::cout << "Run time v0 " << v0 / runtime << std::endl;

    for (int i = 0; i < runtime; i++) {
        v1 += thread_v1(ranges);
    }
    std::cout << "Run time v1 " << v1 / runtime << std::endl;

    return 0;
}