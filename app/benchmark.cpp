#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

#include "../src/v0/range_lock.hpp"
#include "../src/v1/range_lock_v1.cpp"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define DEF "\033[0m"

constexpr uint64_t numThreads = 10;
constexpr uint64_t range = 100000;

double thread_v0_debug()
{
    ConcurrentRangeLock<uint64_t, 10> crl{};
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&crl, i, &printMutex]
                             {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;

                printMutex.lock();
                std::cout << "thread " << i << " try locking " << start << " " << end << std::endl;
                printMutex.unlock();

                bool locked = false;
                
                locked = crl.tryLock(start, end);

                if (locked) {
                    printMutex.lock();
                    std::cout << "thread " << i << GREEN << " locked successfully " << DEF << start << " " << end << std::endl;
                    printMutex.unlock();
                } else {
                    printMutex.lock();
                    std::cout << "thread " << i << " failed to lock " << start << " " << end << std::endl;
                    printMutex.unlock();
                }

                if (locked) {
                    printMutex.lock();
                    std::cout << "thread " << i << " try releasing " << start << " " << end << std::endl;
                    printMutex.unlock();
                    
                    if (crl.releaseLock(start, end)) {
                        printMutex.lock();
                        std::cout << "thread " << i << RED << " released successfully " << DEF << start << " " << end << std::endl;
                        printMutex.unlock();
                    } else {
                        printMutex.lock();
                        std::cout << "thread " << i << " failed to release " << start << " " << end << std::endl;
                        printMutex.unlock();
                    }
                }

            } });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread : threads)
    {
        thread.join();
    }

    crl.displayList();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

double thread_v0()
{
    ConcurrentRangeLock<uint64_t, 10> crl{};
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&crl, i, &printMutex]
                             {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;

                bool locked = crl.tryLock(start, end);

                std::cout << GREEN << "thread " << i << " " << start << " " << end << DEF<< std::endl; 

                if (locked) {
                    crl.releaseLock(start, end);
                }

            } });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread : threads)
    {
        thread.join();
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}


double thread_v1()
{
    ListRL list;
    std::mutex printMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&list, i, &printMutex]
                             {
            for (uint64_t j = 0; j < range; j += 10) {
                auto start = j;
                auto end = j + 5;

                auto rl = MutexRangeAcquire(&list, start, end);

                MutexRangeRelease(rl);
            } });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto &thread : threads)
    {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

void test()
{
    int num_elements = 1000;
    ConcurrentRangeLock<int, 16> crl{};

    for (int i = 0; i < num_elements; i += 2)
    {
        crl.tryLock(i, i + 1);
    }

    auto releaseLockFunc = [&](int start, int end)
    {
        for (int i = start; i < end; i += 2)
        {
            auto res = crl.releaseLock(i, i + 1);
            assert(res == true);
        }
    };

    std::vector<std::thread> threads;

    threads.emplace_back(releaseLockFunc, 0, 500);
    threads.emplace_back(releaseLockFunc, 500, num_elements);

    for (auto &t : threads)
    {
        t.join();
    }

    assert(crl.size() == 0);
    std::cout << crl.size() << std::endl;
}

void simple() {
    ConcurrentRangeLock<int, 16> crl{};

    for (int i = 0; i < 50; i += 10)
    {
        auto res = crl.tryLock(i, i + 5);
        assert(res == true);

        crl.displayList();

        auto res2 = crl.releaseLock(i, i + 5);
        assert(res2 == true);

        crl.displayList();
    }
}

int main()
{
    int runtime = 1;

    // simple();

    std::cout << "Multiple thread" << std::endl;
    double total = 0;

    for (int i = 0; i < runtime; i++)
    {
        total += thread_v0_debug();
    }
    std::cout << "Run time v0 " << total / runtime << std::endl;

    // total = 0;
    // for (int i = 0; i < runtime; i++)
    // {
    //     total += thread_v1();
    // }
    // std::cout << "Run time v0 " << total / runtime << std::endl;

    return 0;
}
