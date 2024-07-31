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
#include "../src/v3/range_lock.cpp"
#include "/usr/local/Cellar/google-benchmark/1.8.5/include/benchmark/benchmark.h"

constexpr int minThreads = 1;
constexpr int maxThreads = 16;
constexpr int rangeStart = 1;
constexpr int rangeEnd = 100000;
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

void runScalabilityV0(benchmark::State& state) {
    int numThreads = state.range(0);
    ConcurrentRangeLock<uint64_t, 6> crl{};
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    std::barrier syncPoint(numThreads + 1);

    auto ranges = createNonOverlappingRanges();
    auto rangePerThread = ranges.size() / numThreads;

    for (auto _ : state) {
        threads.clear();
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

        syncPoint.arrive_and_wait();
        auto start = std::chrono::steady_clock::now();
        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        assert(crl.size() == ranges.size());
        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());

        // Add custom counters
        state.counters["Locks"] = crl.size();
    }
}

void runScalabilityV2(benchmark::State& state) {
    int numThreads = state.range(0);
    ListRL list;
    std::vector<std::thread> threads;
    std::barrier syncPoint(numThreads + 1);

    auto ranges = createNonOverlappingRanges();
    auto rangePerThread = ranges.size() / numThreads;

    for (auto _ : state) {
        threads.clear();
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

        syncPoint.arrive_and_wait();
        auto start = std::chrono::steady_clock::now();
        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        assert(list.size() == ranges.size());
        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());

        // Add custom counters
        state.counters["Locks"] = list.size();
    }
}

void runScalabilityV3(benchmark::State& state) {
    int numThreads = state.range(0);
    SongRangeLock rl;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    std::barrier syncPoint(numThreads + 1);

    auto ranges = createNonOverlappingRanges();
    auto rangePerThread = ranges.size() / numThreads;

    for (auto _ : state) {
        threads.clear();
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([&, i]() {
                syncPoint.arrive_and_wait();

                auto startIdx = i * rangePerThread;
                auto endIdx = (i == numThreads - 1) ? ranges.size()
                                                    : startIdx + rangePerThread;

                for (auto j = startIdx; j < endIdx; ++j) {
                    rl.tryLock(ranges[j].first, ranges[j].second);
                }
            });
        }

        syncPoint.arrive_and_wait();
        auto start = std::chrono::steady_clock::now();
        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        assert(rl.size() == ranges.size());
        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());

        // Add custom counters
        state.counters["Locks"] = rl.size();
    }
}

BENCHMARK(runScalabilityV0)
    ->RangeMultiplier(2)
    ->Range(minThreads, maxThreads)
    ->UseManualTime()
    ->Iterations(5);
BENCHMARK(runScalabilityV2)
    ->RangeMultiplier(2)
    ->Range(minThreads, maxThreads)
    ->UseManualTime()
    ->Iterations(5);
BENCHMARK(runScalabilityV3)
    ->RangeMultiplier(2)
    ->Range(minThreads, maxThreads)
    ->UseManualTime()
    ->Iterations(5);

BENCHMARK_MAIN();