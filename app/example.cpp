#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

#include "../src/concurrentSkipList.cpp"

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

void thread() {
    ConcurrentRangeLock<uint16_t, 4> crl{};
    const int numThreads = 20;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, &i] {
            for (uint16_t k = 0; k < 8; ++k) {
                if (crl.tryLock(k * i, k * i + 5) == true) {
                    std::cout << "Locked " << k * i << "-" << k * i + 5
                              << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                crl.releaseLock(k * i, k * i + 5);
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    crl.displayList();
}

int main() {
    std::cout << "Basic functionality " << std::endl;
    basic();

    std::cout << "Multiple thread" << std::endl;
    thread();
}