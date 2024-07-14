#include <algorithm>
#include <atomic>
#include <cassert>
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
constexpr uint64_t range = 800;
constexpr uint16_t lockHeight = 5;
constexpr int workingTime = 0;
constexpr int runtime = 1;

double singleThread_v0(std::vector<std::pair<int, int>> &ranges) {
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};

    auto start = std::chrono::steady_clock::now();

    for (size_t j = 0; j < ranges.size(); ++j) {
        if (crl.tryLock(ranges[j].first, ranges[j].second)) {
            // std::cout << "locled " << ranges[j].first << " " <<
            // ranges[j].second
            //           << std::endl;
        }
    }

    for (size_t j = 0; j < ranges.size(); ++j) {
        crl.releaseLock(ranges[j].first, ranges[j].second);
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

double singleThread_v1(std::vector<std::pair<int, int>> &ranges) {
    ListRL list;
    std::vector<RangeLock *> lockHandles;

    auto start = std::chrono::steady_clock::now();

    for (size_t j = 0; j < ranges.size(); ++j) {
        auto rl = MutexRangeAcquire(&list, ranges[j].first, ranges[j].second);
        lockHandles.push_back(rl);
    }

    // printList(&list);

    for (auto rl: lockHandles) {
        MutexRangeRelease(&list,rl);
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;

    return duration.count();
}

void test() {
    ListRL list;
    auto rl1 = MutexRangeAcquire(&list, 0, 10);
    auto rl2 = MutexRangeAcquire(&list, 10, 20);
    auto rl3 = MutexRangeAcquire(&list, 10, 20);
    printList(&list);
}

void test2() {
    ConcurrentRangeLock<uint64_t, 5> crl{};
    std::vector<std::thread> threads;

    threads.emplace_back([&crl] { crl.tryLock(10, 20); });

    threads.emplace_back([&crl] { crl.tryLock(10, 20); });

    for (auto &thread: threads) {
        thread.join();
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

std::vector<std::pair<int, int>> createRanges(int x) {
    std::vector<std::pair<int, int>> ranges;
    for (int i = 0; i < x; i += 10) {
        ranges.push_back(std::make_pair(i, i + 20));
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(ranges.begin(), ranges.end(),
                 std::default_random_engine(seed));

    return ranges;
}

double thread_v0_debug() {
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};
    std::mutex printMutex;

    auto ranges = createRanges(10000000);

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, &printMutex, &ranges, i] {
            for (size_t j = i; j < ranges.size(); j += numThreads) {
                auto start = ranges[j].first;
                auto end = ranges[j].second;

                printMutex.lock();
                std::cout << "thread " << i << " try locking " << start << " "
                          << end << std::endl;
                printMutex.unlock();

                bool res = crl.tryLock(start, end);
                while (res != true) {
                    res = crl.tryLock(start, end);
                }

                printMutex.lock();
                std::cout << "thread " << i << GREEN << " locked successfully "
                          << DEF << start << " " << end << std::endl;
                std::cout << "thread " << i << " try releasing " << start << " "
                          << end << std::endl;
                printMutex.unlock();

                std::this_thread::sleep_for(
                        std::chrono::microseconds(((start + end) * 1000) % 10000));

                auto y = crl.releaseLock(start, end);

                std::cout << "thread " << i << RED << " released "
                          << (y ? "successfully" : "failed") << " " << DEF
                          << start << " " << end << std::endl;
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread: threads) {
        thread.join();
    }

    crl.displayList();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

int main() {
    thread_v0_debug();
    return 0;
}