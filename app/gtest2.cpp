#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include <utility>
#include <cassert>

#include "../src/v0/range_lock.hpp"

constexpr int numOfRanges = 100000;
constexpr int size = 4;
constexpr int numIterations = 10; 
int numThreads = 8;

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
void runScalabilityWithHeight() {
    std::vector<double> durations(numIterations);

    for (int iter = 0; iter < numIterations; ++iter) {
        ConcurrentRangeLock<uint64_t, HEIGHT> crl{};
        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        auto ranges = createNonOverlappingRanges();
        auto rangePerThread = ranges.size() / numThreads;

        // Measuring time
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([&, i]() {
                auto startIdx = i * rangePerThread;
                auto endIdx = (i == numThreads - 1) ? ranges.size() : startIdx + rangePerThread;

                // Uncomment the case you want to test

                // Case 1
                // for (auto j = startIdx; j < endIdx; ++j) {
                //     crl.tryLock(ranges[j].first, ranges[j].second);
                // }
                // for (auto j = startIdx; j < endIdx; ++j) {
                //     crl.releaseLock(ranges[j].first, ranges[j].second);
                // }

                // Case 2
                // for (auto j = startIdx; j < endIdx; ++j) {
                //     crl.tryLock(ranges[j].first, ranges[j].second);
                //     crl.releaseLock(ranges[j].first, ranges[j].second);
                // }

                // Case 3
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
        durations[iter] = duration.count();
    }

    // Calculate average duration
    double sum = 0;
    for (double duration : durations) {
        sum += duration;
    }
    double averageDuration = sum / numIterations;

    std::cout << "Height: " << HEIGHT << ", Average Time: " << averageDuration << " seconds" << std::endl;
}

int main() {
    // Test different heights
    runScalabilityWithHeight<2>();
    runScalabilityWithHeight<3>();
    runScalabilityWithHeight<4>();
    runScalabilityWithHeight<5>();
    runScalabilityWithHeight<6>();
    runScalabilityWithHeight<7>();
    runScalabilityWithHeight<8>();
    runScalabilityWithHeight<9>();
    runScalabilityWithHeight<10>();
    runScalabilityWithHeight<11>();
    runScalabilityWithHeight<12>();
    runScalabilityWithHeight<13>();
    runScalabilityWithHeight<14>();
    runScalabilityWithHeight<15>();
    runScalabilityWithHeight<16>();
    runScalabilityWithHeight<17>();
    runScalabilityWithHeight<18>();
    runScalabilityWithHeight<19>();
    runScalabilityWithHeight<20>();
    runScalabilityWithHeight<21>();
    runScalabilityWithHeight<22>();
    runScalabilityWithHeight<23>();
    runScalabilityWithHeight<24>();
    runScalabilityWithHeight<25>();
    runScalabilityWithHeight<26>();
    runScalabilityWithHeight<27>();
    runScalabilityWithHeight<28>();
    runScalabilityWithHeight<29>();
    runScalabilityWithHeight<30>();

    return 0;
}
