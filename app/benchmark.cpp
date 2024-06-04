#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../src/v0/range_lock.hpp"
#include "../src/v1/range_lock_v1.cpp"

constexpr uint64_t numThreads = 30;
constexpr uint64_t range = 1000;

void savePrint(std::mutex printMutex, std::string str) {
    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << str << std::endl;
}

void basic_v0() {
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

double thread_v0() {
    ConcurrentRangeLock<uint64_t, 16> crl{};

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, &i] {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;

                crl.tryLock(start, end);
                // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                crl.releaseLock(start, end);
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

double thread_v1() {
    ListRL list;
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&list, i, &printMutex] {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;

                // savePrint(printMutex,
                //           "Thread " + std::to_string(i) + " with id " +
                //               std::to_string(std::this_thread::get_id()) +
                //               " locking " + std::to_string(start) + "-" +
                //               std::to_string(end));

                auto rl = MutexRangeAcquire(&list, start, end);

                // savePrint(printMutex,
                //           "Thread " + std::to_string(i) + " with id " +
                //               std::to_string(std::this_thread::get_id()) +
                //               " locked " + std::to_string(start) + "-" +
                //               std::to_string(end));

                // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                MutexRangeRelease(rl);

                // savePrint(printMutex,
                //           "Thread " + std::to_string(i) + " with id " +
                //               std::to_string(std::this_thread::get_id()) +
                //               " released " + std::to_string(start) + "-" +
                //               std::to_string(end));
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
    int runtime = 1;

    std::cout << "Multiple thread" << std::endl;
    double total = 0;
    for (int i = 0; i < runtime; i++) {
        total += thread_v0();
    }
    std::cout << "Run time v0 " << total / runtime << std::endl;

    total = 0;
    for (int i = 0; i < runtime; i++) {
        total += thread_v1();
    }
    std::cout << "Run time v1 " << total / runtime << std::endl;

    return 0;
}