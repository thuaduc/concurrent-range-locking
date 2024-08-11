#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "../src/v0/range_lock.hpp"
#include "/usr/local/Cellar/google-benchmark/1.8.5/include/benchmark/benchmark.h"

constexpr int numOfRanges = 1000000;
constexpr int size = 4;

int random_with_probability() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < 0.05 ? 0 : 1;
}

std::vector<std::pair<int, int>> createNonOverlappingRanges() {
    std::vector<std::pair<int, int>> ranges;
    int k = 1;
    for (int i = 0; i < numOfRanges; i++) {
        ranges.emplace_back(k, k + size);
        k += (size + 1);
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(ranges.begin(), ranges.end(),
                 std::default_random_engine(seed));
    return ranges;
}

template <int HEIGHT>
void runScalabilityWithHeight(benchmark::State& state) {
    int numThreads = 8;
    ConcurrentRangeLock<uint64_t, HEIGHT> crl{};
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    auto ranges = createNonOverlappingRanges();
    auto rangePerThread = ranges.size() / numThreads;

    for (auto _ : state) {
        threads.clear();

        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([&, i]() {
                auto startIdx = i * rangePerThread;
                auto endIdx = (i == numThreads - 1) ? ranges.size()
                                                    : startIdx + rangePerThread;

                // case 1
                // for (auto j = startIdx; j < endIdx; ++j) {
                //     crl.tryLock(ranges[j].first, ranges[j].second);
                // }

                // for (auto j = startIdx; j < endIdx; ++j) {
                //     crl.releaseLock(ranges[j].first, ranges[j].second);
                // }

                // case 2
                // for (auto j = startIdx; j < endIdx; ++j) {
                //     crl.tryLock(ranges[j].first, ranges[j].second);
                //     crl.releaseLock(ranges[j].first, ranges[j].second);
                // }

                // case 3
                for (auto j = startIdx; j < endIdx; ++j) {
                    crl.tryLock(ranges[j].first, ranges[j].second);

                    if (random_with_probability() == 1) {
                        crl.releaseLock(ranges[j].first, ranges[j].second);
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());

        // std::cout << crl.size() << std::endl;
    }
}

// Register the benchmark for each height from 1 to 16
#define REGISTER_HEIGHT_BENCHMARK(HEIGHT) \
    BENCHMARK_TEMPLATE(runScalabilityWithHeight, HEIGHT)->Iterations(20);

// REGISTER_HEIGHT_BENCHMARK(2)
// REGISTER_HEIGHT_BENCHMARK(3)
REGISTER_HEIGHT_BENCHMARK(4)
REGISTER_HEIGHT_BENCHMARK(5)
REGISTER_HEIGHT_BENCHMARK(6)
REGISTER_HEIGHT_BENCHMARK(7)
REGISTER_HEIGHT_BENCHMARK(8)
REGISTER_HEIGHT_BENCHMARK(9)
REGISTER_HEIGHT_BENCHMARK(10)
REGISTER_HEIGHT_BENCHMARK(11)
REGISTER_HEIGHT_BENCHMARK(12)
REGISTER_HEIGHT_BENCHMARK(13)
REGISTER_HEIGHT_BENCHMARK(14)
REGISTER_HEIGHT_BENCHMARK(15)
REGISTER_HEIGHT_BENCHMARK(16)
REGISTER_HEIGHT_BENCHMARK(17)
REGISTER_HEIGHT_BENCHMARK(18)
REGISTER_HEIGHT_BENCHMARK(19)
REGISTER_HEIGHT_BENCHMARK(20)
REGISTER_HEIGHT_BENCHMARK(21)
REGISTER_HEIGHT_BENCHMARK(22)
REGISTER_HEIGHT_BENCHMARK(23)
REGISTER_HEIGHT_BENCHMARK(24)
REGISTER_HEIGHT_BENCHMARK(25)
REGISTER_HEIGHT_BENCHMARK(26)
REGISTER_HEIGHT_BENCHMARK(27)
REGISTER_HEIGHT_BENCHMARK(28)
REGISTER_HEIGHT_BENCHMARK(29)
REGISTER_HEIGHT_BENCHMARK(30)

BENCHMARK_MAIN();