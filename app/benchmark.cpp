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
    runTest(perf);

    return 0;
}