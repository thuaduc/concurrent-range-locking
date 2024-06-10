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

#define GREEN "\033[32m"
#define RED "\033[31m"
#define DEF "\033[0m"

constexpr uint64_t numThreads = 20;
constexpr uint64_t range = 100000;
constexpr uint16_t lockHeight = 5;
constexpr int workingTime = 100;
constexpr int runTime = 1;

double singleThread_v0(std::vector<std::pair<int, int>> &ranges) {
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};

    auto start = std::chrono::steady_clock::now();

    for (size_t j = 0; j < ranges.size(); ++j) {
        bool locked = crl.tryLock(ranges[j].first, ranges[j].second);
    }

    // for (size_t j = 0; j < ranges.size(); ++j) {
    //     bool locked = crl.releaseLock(ranges[j].first, ranges[j].second);
    // }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

double singleThread_v1(std::vector<std::pair<int, int>> &ranges) {
    ListRL list;
    std::vector<std::shared_ptr<RangeLock>> lockHandles;

    auto start = std::chrono::steady_clock::now();

    for (size_t j = 0; j < ranges.size(); ++j) {
        auto rl = MutexRangeAcquire(&list, ranges[j].first, ranges[j].second);
        lockHandles.push_back(rl);
    }

    // for (auto rl : lockHandles) {
    //     MutexRangeRelease(&list, rl);
    // }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

double thread_v0_debug() {
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, i, &printMutex] {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;

                printMutex.lock();
                std::cout << "thread " << i << " try locking " << start << " "
                          << end << std::endl;
                printMutex.unlock();

                bool locked = false;

                locked = crl.tryLock(start, end);

                if (locked) {
                    printMutex.lock();
                    std::cout << "thread " << i << GREEN
                              << " locked successfully " << DEF << start << " "
                              << end << std::endl;
                    printMutex.unlock();
                } else {
                    printMutex.lock();
                    std::cout << "thread " << i << " failed to lock " << start
                              << " " << end << std::endl;
                    printMutex.unlock();
                }

                if (locked) {
                    printMutex.lock();
                    std::cout << "thread " << i << " try releasing " << start
                              << " " << end << std::endl;
                    printMutex.unlock();

                    if (crl.releaseLock(start, end)) {
                        printMutex.lock();
                        std::cout << "thread " << i << RED
                                  << " released successfully " << DEF << start
                                  << " " << end << std::endl;
                        printMutex.unlock();
                    } else {
                        printMutex.lock();
                        std::cout << "thread " << i << " failed to release "
                                  << start << " " << end << std::endl;
                        printMutex.unlock();
                    }
                }
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread : threads) {
        thread.join();
    }

    crl.displayList();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

void test() {
    int num_elements = 1000;
    const int num_threads = 20;
    const int num_operations_per_thread = 50;
    ConcurrentRangeLock<int, 5> crl{};

    auto mixedOpFunc = [&](int thread_id) {
        for (int i = 0; i < num_operations_per_thread; i += 2) {
            int value = thread_id * num_operations_per_thread + i;

            // Even thread IDs insert, odd thread IDs search
            // All threads attempt to delete
            if (thread_id % 2 == 0) {
                crl.tryLock(value, value + 1);
            } else {
                crl.searchLock(value, value + 1);
            }
            crl.releaseLock(value, value + 1);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(mixedOpFunc, i);
    }

    for (auto &t : threads) {
        t.join();
    }
}

void simple() {
    ConcurrentRangeLock<int, 8> crl{};

    for (int i = 150; i > 0; i -= 10) {
        auto res = crl.tryLock(i - 5, i);
        assert(res == true);
    }
    crl.displayList();
}

void shuffleVector(std::vector<std::pair<int, int>> &vec) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(vec.begin(), vec.end(), std::default_random_engine(seed));
}

std::vector<std::pair<int, int>> divideIntoRanges(int x) {
    std::vector<std::pair<int, int>> ranges;
    for (int i = 0; i < x; i += 5) {
        ranges.push_back(std::make_pair(i, std::min(i + 5, x)));
    }
    shuffleVector(ranges);
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

                bool locked = crl.tryLock(start, end);

                if (locked) {
                    // Simulate work
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(workingTime));
                    crl.releaseLock(start, end);
                }
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

                // Simulate work
                std::this_thread::sleep_for(
                    std::chrono::microseconds(workingTime));
                MutexRangeRelease(&list, rl);
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
    // test();

    std::cout << "Multiple thread" << std::endl;
    double total = 0;

    std::vector<std::pair<int, int>> ranges = divideIntoRanges(range);

    for (int i = 0; i < runTime; i++) {
        total += thread_v1(ranges);
    }
    std::cout << "Run time v1 " << total / runTime << std::endl;

    total = 0;

    for (int i = 0; i < runTime; i++) {
        total += thread_v0(ranges);
    }
    std::cout << "Run time v0 " << total / runTime << std::endl;

    return 0;
}