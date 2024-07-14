#include <fstream>
#include <iostream>
#include <random>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../src/v1/range_lock.cpp"
#include "../src/v2/range_lock.hpp"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define DEF "\033[0m"

constexpr uint16_t lockHeight = 4;

// Simulated database table
std::unordered_map<int, std::string> database;
std::uniform_int_distribution<int> dist(0, 9'999'000);
std::uniform_int_distribution<int> range_dist(10, 1000);
const int num_records = 10'000'000;
const int num_transactions_per_thread = 10000;
const int time_delay = 1;
std::mutex printMutex;
// std::ofstream logFile("log.txt");

// Initialize database with dummy data
void initialize_database(int num_records) {
    for (int i = 0; i < num_records; ++i) {
        database[i] = "data_" + std::to_string(i);
    }
}

// Function to simulate a database transaction for v0
void database_transaction_v0(ConcurrentRangeLock<uint64_t, lockHeight> &crl,
                             int thread_id, int num_transactions) {
    std::mt19937 rng(thread_id);

    for (int i = 0; i < num_transactions; ++i) {
        int start = dist(rng);
        int end = start + range_dist(rng);

        while (!crl.tryLock(start, end)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(time_delay));
        }

        /*printMutex.lock();
        logFile << "thread " << thread_id << GREEN << " locked " << DEF << start << " " << end << std::endl;
        printMutex.unlock();*/

        // Simulate read/write operation
        for (int key = start; key < end; ++key) {
            database[key] = "updated_by_thread_" + std::to_string(thread_id);
        }

        if (!crl.releaseLock(start, end)){
            crl.displayList();
            exit(EXIT_FAILURE);
        }

       /* printMutex.lock();
        logFile << "thread " << thread_id << RED << " released" << " " << DEF << start << " " << end << std::endl;
        printMutex.unlock();*/
    }
}

// Function to simulate a database transaction for v1
void database_transaction_v1(ListRL &list, int thread_id,
                             int num_transactions) {
    std::mt19937 rng(thread_id);

    for (int i = 0; i < num_transactions; ++i) {
        int start = dist(rng);
        int end = start + range_dist(rng);

        auto rl = MutexRangeAcquire(&list, start, end);

        while (rl == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(time_delay));
            rl = MutexRangeAcquire(&list, start, end);
        }

        // Simulate read/write operation
        for (int key = start; key < end; ++key) {
            database[key] = "updated_by_thread_" + std::to_string(thread_id);
        }

        MutexRangeRelease(&list,rl);
    }
}

// Benchmark function of v0 range lock
auto benchmark_v0(int num_threads, int num_transactions_per_thread) {
    std::vector<std::thread> threads;
    ConcurrentRangeLock<uint64_t, lockHeight> crl{};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(database_transaction_v0, std::ref(crl), i, num_transactions_per_thread);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto &thread: threads) {
        thread.join();
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> total_time = end_time - start_time;
    std::cout << "Num threads: " << num_threads << ". Total time taken v0: " << total_time.count() << " seconds"
              << std::endl;


    return total_time.count();
}

// Benchmark function of v1 range lock
auto benchmark_v1(int num_threads, int num_transactions_per_thread) {
    std::vector<std::thread> threads;
    ListRL list;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(database_transaction_v1, std::ref(list), i, num_transactions_per_thread);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto &thread: threads) {
        thread.join();
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> total_time = end_time - start_time;
    std::cout << "Num threads: " << num_threads << ". Total time taken v1: " << total_time.count() << " seconds"
              << std::endl;


    return total_time.count();
}

int main() {
    initialize_database(num_records);

    for (int i = 10; i <= 50; i += 10) {
        benchmark_v0(i, num_transactions_per_thread);
        benchmark_v1(i, num_transactions_per_thread);
        std::cout << std::endl;
    }

    // logFile.close();
    return 0;
}