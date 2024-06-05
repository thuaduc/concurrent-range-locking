#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../src/v0/range_lock.hpp"
#include "../src/v1/range_lock_v1.cpp"

constexpr uint64_t numThreads = 10;
constexpr uint64_t range = 1000;

double thread_v0() {
    ConcurrentRangeLock<uint64_t, 16> crl{};
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&crl, i, &printMutex] {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;
                
                printMutex.lock();
                std::cout << "thread " << i <<" locking " << start << " " << end << std::endl;
                printMutex.unlock();
                
                crl.tryLock(start, end);
                
                printMutex.lock();
                std::cout << "thread " << i <<" locked " << start << " " << end << std::endl;
                printMutex.unlock();
                
                // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                crl.releaseLock(start, end);

                printMutex.lock();
                std::cout << "thread " << i << " released " << start << " " << end << std::endl;
                printMutex.unlock();

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

    // total = 0;
    // for (int i = 0; i < runtime; i++) {
    //     total += thread_v1();
    // }
    // std::cout << "Run time v1 " << total / runtime << std::endl;

    // return 0;
}