#include <algorithm>
#include <atomic>
#include <barrier>
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "../src/v0/range_lock.hpp"
#include "../src/v2/range_lock.cpp"

constexpr int minThreads = 1;
constexpr int maxThreads = 17;
constexpr int rangeStart = 1;
constexpr int rangeEnd = 500000;
constexpr int runtimes = 10;
constexpr int step = 4;

std::vector<std::pair<int, int>> createNonOverlappingRanges() {
    std::vector<std::pair<int, int>> ranges;
    for (int i = rangeStart; i < rangeEnd; i += 10) {
        ranges.emplace_back(i, i + 5);
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(ranges.begin(), ranges.end(),
                 std::default_random_engine(seed));
    return ranges;
}

double runScalabilityV0(int numThreads,
                        const std::vector<std::pair<int, int>> &ranges) {
    ConcurrentRangeLock<uint64_t, 6> crl{};
    std::vector<std::thread> threads;
    std::barrier syncPoint(numThreads + 1);

    auto rangePerThread = ranges.size() / numThreads;

    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([&, i]() {
            syncPoint.arrive_and_wait();

            auto startIdx = i * rangePerThread;
            auto endIdx = (i == numThreads - 1) ? ranges.size()
                                                : startIdx + rangePerThread;

            for (auto j = startIdx; j < endIdx; ++j) {
                crl.tryLock(ranges[j].first, ranges[j].second);
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    syncPoint.arrive_and_wait();
    for (auto &thread : threads) {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;

    assert(crl.size() == ranges.size());
    return static_cast<double>(crl.size()) / duration.count();
}

double runScalabilityV2(int numThreads,
                        const std::vector<std::pair<int, int>> &ranges) {
    ListRL list;
    std::vector<std::thread> threads;
    std::barrier syncPoint(numThreads + 1);

    auto rangePerThread = ranges.size() / numThreads;

    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([&, i]() {
            syncPoint.arrive_and_wait();

            auto startIdx = i * rangePerThread;
            auto endIdx = (i == numThreads - 1) ? ranges.size()
                                                : startIdx + rangePerThread;

            for (auto j = startIdx; j < endIdx; ++j) {
                MutexRangeAcquire(&list, ranges[j].first, ranges[j].second);
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    syncPoint.arrive_and_wait();
    for (auto &thread : threads) {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;

    assert(list.size() == ranges.size());
    return static_cast<double>(list.size()) / duration.count();
}

int main() {
    std::ofstream outFile("data/scalability_benchmark.txt", std::ios_base::app);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file!" << std::endl;
        return 1;
    }

    auto ranges = createNonOverlappingRanges();

    std::cout << "V0:\n";
    for (int numThreads = minThreads; numThreads <= maxThreads;
         numThreads += step) {
        std::cout << "Threads: " << numThreads << "\n";
        outFile << "Threads: " << numThreads << "\n";

        double total = 0;
        for (int i = 0; i < 10; i++) {
            total += runScalabilityV0(numThreads, ranges);
        }
        double average = total / 10;

        std::cout << "Average locks per second: " << average << "\n";
        outFile << "Average locks per second: " << average << "\n";
        std::cout << "----------------------------------\n";
    }

    std::cout << "V2:\n";
    for (int numThreads = minThreads; numThreads <= maxThreads;
         numThreads += step) {
        std::cout << "Threads: " << numThreads << "\n";
        outFile << "Threads: " << numThreads << "\n";

        double total = 0;
        for (int i = 0; i < runtimes; i++) {
            total += runScalabilityV2(numThreads, ranges);
        }
        double average = total / runtimes;

        std::cout << "Average locks per second: " << average << "\n";
        outFile << "Average locks per second: " << average << "\n";
        std::cout << "----------------------------------\n";
    }

    outFile.close();
    return 0;
}