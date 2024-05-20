#include <algorithm>
#include <csignal>
#include <fstream>
#include <string>
#include <vector>
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

#include "../inc/range_lock.hpp"
#include "../inc/perf.hpp"


void threadTask(ConcurrentRangeLock<uint32_t, 20>& crl, uint32_t start, uint32_t end, PerfEvent& perf, int numthread) {
    {
        PerfEventBlock peb(perf, (end - start) / 2, {"try_lock    " + std::to_string(numthread)});
        for (uint32_t i = start; i < end; i += 2) {
            crl.tryLock(i, i + 1);
        }
    } 
    
    {
        PerfEventBlock peb(perf, (end - start) / 2, {"search_lock "+ std::to_string(numthread)});
        for (uint32_t i = start; i < end; i += 2) {
            crl.searchLock(i, i + 1);
        }
    } 
    
    {
        PerfEventBlock peb(perf, (end - start) / 2, {"release_lock"+ std::to_string(numthread)});
        for (uint32_t i = start; i < end; i += 2) {
            crl.releaseLock(i, i + 1);
        }
    }
    std::cout << std::endl;
}

void runTestThread(PerfEvent& perf) {
    ConcurrentRangeLock<uint32_t, 20> crl{};
    uint32_t length = 100000;
    int numThreads = 5;
    std::vector<std::thread> threads;

    // Divide the work among 10 threads
    uint32_t block_size = length / numThreads;

    for (int t = 0; t < numThreads; ++t) {
        uint32_t start = t * block_size;
        uint32_t end = (t + 1) * block_size;

        // Ensure the last thread covers any remaining range
        if (t == numThreads - 1) {
            end = length;
        }

        // Launch threads for each type of lock operation
        threads.emplace_back(threadTask, std::ref(crl), start, end, std::ref(perf), t);
        threads.emplace_back(threadTask, std::ref(crl), start, end, std::ref(perf), t);
        threads.emplace_back(threadTask, std::ref(crl), start, end, std::ref(perf), t);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
}


void runTest(PerfEvent& perf) {
    ConcurrentRangeLock<uint32_t, 20> crl{};
    uint32_t length = 100000;

    {
        PerfEventBlock peb(perf,length/2,{"try_lock    "});
        for (uint32_t i = 0; i < length; i+=2) {
            crl.tryLock(i, i+1);
        }
    }

    {
        PerfEventBlock peb(perf,length/2,{"search_lock "});
         for (uint32_t i = 0; i < length; i+=2) {
            crl.searchLock(i, i+1);
        }
    }

    {
        PerfEventBlock peb(perf,length/2,{"release_lock"});
        for (uint32_t i = 0; i < length; i+=2) {
            crl.releaseLock(i, i+1);
        }
    }
}

int main() {
    srand(42);
    PerfEvent perf;
    runTestThread(perf);

    return 0;
}