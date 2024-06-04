#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../src/v1/range_lock_v1.cpp"

// void basic() {
//     ListRL list;
//     uint16_t length = 10;

//     std::vector<std::shared_ptr<RangeLock>> locks;
//     for (uint16_t i = length; i >= 1; i--) {
//         auto rl = MutexRangeAcquire(&list, i * 10, i * 10 + 5);
//         locks.push_back(rl);
//     }

//     for (uint16_t i = length; i >= 1; i--) {
//         assert(locks[length - i]->node->start == i * 10);
//         assert(locks[length - i]->node->end == i * 10 + 5);
//     }

//     for (auto& lock : locks) {
//         MutexRangeRelease(lock);
//     }
// }

double thread() {
    ListRL list;
    int numThreads = 5;
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&list, i, numThreads, &printMutex] {
            for (uint64_t j = 0; j < numThreads * 10; j += 10) {
                auto start = j;
                auto end = j + 9;

                printMutex.lock();
                std::cout << "Thread " << i << " with id "
                          << std::this_thread::get_id() << " locking " << start
                          << "-" << end << std::endl;
                printMutex.unlock();

                auto rl = MutexRangeAcquire(&list, start, end);

                printMutex.lock();
                std::cout << "Thread " << i << " with id "
                          << std::this_thread::get_id() << " locked " << start
                          << "-" << end << std::endl;
                printMutex.unlock();

                // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                MutexRangeRelease(rl);

                printMutex.lock();
                std::cout << "Thread " << i << " with id "
                          << std::this_thread::get_id() << " released " << start
                          << "-" << end << std::endl;
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

int main() {
    // std::cout << "Basic functionality " << std::endl;
    // basic();

    std::cout << "Multiple thread" << std::endl;
    double total = 0;
    // for (int i = 0; i < 1; i++) {
    total += thread();
    //}
    std::cout << "Run time v1 " << total << std::endl;

    return 0;
}
