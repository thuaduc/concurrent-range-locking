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
#include "../src/v1/range_lock.hpp"
#include "../src/v2/range_lock.cpp"
#include "../src/v3/range_lock.cpp"
#include "/usr/local/Cellar/google-benchmark/1.8.5/include/benchmark/benchmark.h"

constexpr int minThreads = 1;
constexpr int maxThreads = 16;
constexpr int numOfRanges = 50000;
constexpr int size = 4;
constexpr size_t sharedMemorySize = numOfRanges * (size + 1);

uint8_t* createSharedMemory() {
    // Allocate memory using mmap
    uint8_t* addr = static_cast<uint8_t*>(
        mmap(nullptr, sharedMemorySize, PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_SHARED, -1, 0));
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return addr;
}

void destroySharedMemory(uint8_t* addr) {
    if (munmap(addr, sharedMemorySize) == -1) {
        perror("munmap");
        exit(1);
    }
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

void runScalabilityV0(benchmark::State& state) {
    int numThreads = state.range(0);
    ConcurrentRangeLock<uint64_t, 6> crl{};
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    uint8_t* sharedMemory = createSharedMemory();
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

                for (auto j = startIdx; j < endIdx; ++j) {
                    crl.tryLock(ranges[j].first, ranges[j].second);
                    memset(sharedMemory + ranges[j].first, 1,
                           ranges[j].second - ranges[j].first);
                }

                for (auto j = startIdx; j < endIdx; ++j) {
                    crl.releaseLock(ranges[j].first, ranges[j].second);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());
    }

    destroySharedMemory(sharedMemory);
}

void runScalabilityV1(benchmark::State& state) {
    int numThreads = state.range(0);
    ConcurrentRangeLock_V1<uint64_t, 6> crl{};
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    uint8_t* sharedMemory = createSharedMemory();
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

                for (auto j = startIdx; j < endIdx; ++j) {
                    crl.tryLock(ranges[j].first, ranges[j].second);
                    memset(sharedMemory + ranges[j].first, 1,
                           ranges[j].second - ranges[j].first);
                }
                for (auto j = startIdx; j < endIdx; ++j) {
                    crl.releaseLock(ranges[j].first, ranges[j].second);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());
    }

    destroySharedMemory(sharedMemory);
}

void runScalabilityV2(benchmark::State& state) {
    int numThreads = state.range(0);
    ListRL list;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    uint8_t* sharedMemory = createSharedMemory();
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

                std::vector<RangeLock*> rls;
                rls.reserve(ranges.size());

                for (auto j = startIdx; j < endIdx; ++j) {
                    auto rl = MutexRangeAcquire(&list, ranges[j].first,
                                                ranges[j].second);
                    rls.emplace_back(rl);
                    memset(sharedMemory + ranges[j].first, 1,
                           ranges[j].second - ranges[j].first);
                }

                for (auto rl : rls) {
                    MutexRangeRelease(&list, rl);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());
    }

    destroySharedMemory(sharedMemory);
}

void runScalabilityV3(benchmark::State& state) {
    int numThreads = state.range(0);
    SongRangeLock rl;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    uint8_t* sharedMemory = createSharedMemory();
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

                for (auto j = startIdx; j < endIdx; ++j) {
                    rl.tryLock(ranges[j].first, ranges[j].second);
                    memset(sharedMemory + ranges[j].first, 1,
                           ranges[j].second - ranges[j].first);
                }
                for (auto j = startIdx; j < endIdx; ++j) {
                    rl.releaseLock(ranges[j].first);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> duration = end - start;
        state.SetIterationTime(duration.count());
    }

    destroySharedMemory(sharedMemory);
}

BENCHMARK(runScalabilityV0)
    ->RangeMultiplier(2)
    ->Range(minThreads, maxThreads)
    ->UseManualTime()
    ->Iterations(5);

BENCHMARK(runScalabilityV1)
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