#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <new>
#include <random>
#include <thread>
#include <vector>

#include "../src/v0/range_lock.hpp"
#include "../src/v2/range_lock.cpp"

constexpr uint64_t numThreads = 20;
constexpr uint16_t lockHeight = 5;
constexpr int runtime = 10;
const std::vector<int> testRanges = {100000, 200000, 300000, 400000, 500000};

std::vector<std::pair<int, int>> createRanges(int x) {
    std::vector<std::pair<int, int>> ranges;
    for (int i = 0; i < x; i += 20) {
        ranges.push_back(std::make_pair(i, i + 10));
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(ranges.begin(), ranges.end(),
                 std::default_random_engine(seed));

    return ranges;
}

double thread_v0(std::vector<std::pair<int, int>> &ranges) {
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, &ranges, i] {
            for (size_t j = i; j < ranges.size(); j += numThreads) {
                auto start = ranges[j].first;
                auto end = ranges[j].second;

                bool res = crl.tryLock(start, end);
                // std::this_thread::sleep_for(
                //     std::chrono::microseconds(((start + end) * 100) %
                //     10000));

                // if (res) {
                //     crl.releaseLock(start, end);
                // }
            }
        });
    }

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

                // std::this_thread::sleep_for(
                //     std::chrono::microseconds(((start + end) * 100) %
                //     10000));

                // if (rl != nullptr) {
                //     MutexRangeRelease(&list, rl);
                // }
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
    std::ofstream outFile(
        "/Users/nguyenduc/Downloads/Studium/SS "
        "2024/thesis/concurrent-range-locking/data/benchmark.txt",
        std::ios_base::app);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file!" << std::endl;
        return 1;
    }

    outFile << "Benchmarking results\n";

    for (const auto &range : testRanges) {
        double v0 = 0;
        double v1 = 0;
        std::vector<std::pair<int, int>> ranges = createRanges(range);

        std::cout
            << "\n----------------------------------\nBenchmarking for range: "
            << range << std::endl;
        outFile << "Range: " << range << "\n";

        for (int I = 0; I < runtime; I++) {
            v0 += thread_v0(ranges);
        }
        std::cout << "Run time v0 " << v0 / runtime << std::endl;
        outFile << "Run time v0: " << v0 / runtime << "\n";

        for (int I = 0; I < runtime; I++) {
            v1 += thread_v1(ranges);
        }
        std::cout << "Run time v1 " << v1 / runtime << std::endl;
        outFile << "Run time v1: " << v1 / runtime << "\n";
    }

    outFile.close();
    return 0;
}