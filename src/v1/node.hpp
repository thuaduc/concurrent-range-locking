#pragma once
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <thread>

class OptimisticMutex {
   public:
    OptimisticMutex() : version(0) {}

    void lock() {
        int expectedVersion;
        while (true) {
            expectedVersion = version.load(std::memory_order_acquire);

            // Check if the lock is free (even version number)
            if ((expectedVersion & 1) == 0) {
                // Try to acquire the lock by incrementing the version number
                if (version.compare_exchange_strong(
                        expectedVersion, expectedVersion + 1,
                        std::memory_order_acq_rel)) {
                    break;  // Acquired the lock
                }
            } else {
                // Optional: Use a short spin-wait before retrying
                std::this_thread::yield();
            }
        }
    }

    void unlock() {
        // Increment the version number to release the lock
        version.fetch_add(1, std::memory_order_release);
    }

   private:
    std::atomic<int> version;
};

constexpr uint64_t CacheLineSize = 64;

template <typename T>
struct alignas(CacheLineSize) Node {
    Node(T start, T end, int level);
    ~Node();

    int getTopLevel() const;
    T getStart() const;
    T getEnd() const;

    Node **next;
    bool marked = false;
    bool fullyLinked = false;

    void lock();
    void unlock();

   private:
    T start;
    T end;
    int topLevel;
    // OptimisticMutex mutex;
    std::mutex mutex;
};

template <typename T>
Node<T>::Node(T start, T end, int level)
    : start{start}, end{end}, topLevel{level} {
    next = new Node<T> *[level + 1];
}

template <typename T>
Node<T>::~Node() {
    // delete next;
}

template <typename T>
void Node<T>::lock() {
    mutex.lock();
}

template <typename T>
void Node<T>::unlock() {
    mutex.unlock();
}

template <typename T>
int Node<T>::getTopLevel() const {
    return topLevel;
}

template <typename T>
T Node<T>::getStart() const {
    return start;
}

template <typename T>
T Node<T>::getEnd() const {
    return end;
}