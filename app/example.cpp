#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "../src/v0/range_lock.hpp"

void basic() {
    ConcurrentRangeLock<uint16_t, 4> crl{};
    uint16_t length = 10;

    for (uint16_t i = length - 1; i >= 1; i--) {
        crl.tryLock(i * 10, i * 10 + (i % 6 + 3));
    }

    crl.displayList();

    for (uint16_t i = length - 1; i >= 1; i--) {
        assert(crl.searchLock(i * 10, i * 10 + (i % 6 + 3)) == true);
    }

    std::cout << std::endl;

    for (uint16_t i = length - 1; i >= 1; i--) {
        crl.releaseLock(i * 10, i * 10 + (i % 6 + 3));
    }

    crl.displayList();
}

double thread() {
    ConcurrentRangeLock<uint64_t, 16> crl{};
    int numThreads = 20;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, &i, numThreads] {
            for (uint16_t i = 0; i < numThreads * 4; i += 5) {
                crl.tryLock(i, i + 10);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                crl.releaseLock(i, i + 10);
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto& thread : threads) {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

int main() {
    // std::cout << "Basic functionality " << std::endl;
    // basic();

    std::cout << "Multiple thread" << std::endl;
    double total = 0;
    for (int i = 0; i < 10; i++) {
        total += thread();
    }
    std::cout << "Run time v0 " << total << std::endl;

    return 0;
}