#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

#include "../src/concurrentSkipList.cpp"

void basic() {
    ConcurrentSkipList<uint16_t, uint16_t, 4> csl{};
    uint16_t length = 10;

    for (uint16_t i = length - 1; i >= 1; i--) {
        csl.insertElement(i, i);
    }

    for (uint16_t i = 1; i < length; i++) {
        if (csl.searchElement(i)) {
            std::cout << "True ";
        }
    }
    std::cout << std::endl;

    csl.displayList();

    for (uint16_t i = 1; i < length; i++) {
        csl.deleteElement(i);
    }

    csl.displayList();
}

void thread() {
    ConcurrentSkipList<uint16_t, uint16_t, 4> csl{};
    const int numThreads = 50;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&csl] {
            for (uint16_t k = 0; k < 1; ++k) {
                csl.insertElement(k, k);
                csl.deleteElement(k);
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    csl.displayList();
}

int main() {
    basic();
    std::cout << std::endl;
    thread();
}