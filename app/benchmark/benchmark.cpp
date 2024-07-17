#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "../src/v1/range_lock.cpp"
#include "../src/v2/range_lock.hpp"

constexpr uint16_t lockHeight = 4;

std::vector<std::pair<int, int>> initial_ranges;
std::vector<std::pair<int, int>> instant_ranges;

void read_ranges(const std::string& initial_ranges_file, const std::string& instant_ranges_file) {
    std::ifstream initial_in(initial_ranges_file);
    std::ifstream instant_in(instant_ranges_file);

    int start, end;
    while (initial_in >> start >> end) {
        initial_ranges.emplace_back(start, end);
    }
    while (instant_in >> start >> end) {
        instant_ranges.emplace_back(start, end);
    }
}

// Function to simulate range locking for v0
void range_lock_v0(ConcurrentRangeLock<uint64_t, lockHeight> &crl) {
    // Lock predefined 1 million ranges
    for (const auto& range : initial_ranges) {
        int start = range.first;
        int end = range.second;

        while (!crl.tryLock(start, end)) {
            std::this_thread::yield();
        }
    }

    // TryLock and Release instantly for predefined 100,000 ranges
    for (const auto& range : instant_ranges) {
        int start = range.first;
        int end = range.second;

        while (!crl.tryLock(start, end)) {
            std::this_thread::yield();
        }

        crl.releaseLock(start, end);
    }


    // Release the initial 1 million ranges
    for (const auto& range : initial_ranges) {
        int start = range.first;
        int end = range.second;

        crl.releaseLock(start, end);
    }

}

// Function to simulate range locking for v1
void range_lock_v1(ListRL &list) {
    std::vector<RangeLock*> acquired_locks;

    // Lock predefined 1 million ranges
    for (const auto& range : initial_ranges) {
        int start = range.first;
        int end = range.second;

        auto rl = MutexRangeAcquire(&list, start, end);

        while (rl == nullptr) {
            std::this_thread::yield();
            rl = MutexRangeAcquire(&list, start, end);
        }

        acquired_locks.push_back(rl);
    }

    // TryLock and Release instantly for predefined 100,000 ranges
    for (const auto& range : instant_ranges) {
        int start = range.first;
        int end = range.second;

        auto rl = MutexRangeAcquire(&list, start, end);

        while (rl == nullptr) {
            std::this_thread::yield();
            rl = MutexRangeAcquire(&list, start, end);
        }

        MutexRangeRelease(&list, rl);
    }

    // Release the initial 1 million ranges
    for (auto& rl : acquired_locks) {
        MutexRangeRelease(&list, rl);
    }
}

// Benchmark function of v0 range lock
auto benchmark_v0(int num_threads) {
    std::vector<std::thread> threads;
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(range_lock_v0, std::ref(crl));
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto &thread : threads) {
        thread.join();
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> total_time = end_time - start_time;
    std::cout << "Num threads: " << num_threads << ". Total time taken v0: " << total_time.count() << " seconds"
              << std::endl;

    return total_time.count();
}

// Benchmark function of v1 range lock
auto benchmark_v1(int num_threads) {
    std::vector<std::thread> threads;
    ListRL list;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(range_lock_v1, std::ref(list));
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto &thread : threads) {
        thread.join();
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> total_time = end_time - start_time;
    std::cout << "Num threads: " << num_threads << ". Total time taken v1: " << total_time.count() << " seconds"
              << std::endl;

    return total_time.count();
}

int main() {
    read_ranges("initial_ranges.txt", "instant_ranges.txt");

    std::cout << "Done with read range; Start benchmark" << std::endl;

    benchmark_v0(1);
    benchmark_v1(1);

    /*for (int i = 10; i <= 10; i += 10) {
        benchmark_v0(i);
        benchmark_v1(i);
        std::cout << std::endl;
    }*/

    return 0;
}