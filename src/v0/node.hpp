#pragma once
#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <atomic>
#include <thread>

template <typename T>
struct Node {
    Node(T start, T end, int level);
    ~Node() = default;

    T getStart() const;
    T getEnd() const;
    void setStart(T newStart);
    void setEnd(T newEnd);
    void info();

    std::vector<std::shared_ptr<Node<T>>> next;
    std::atomic<bool> marked = false;
    bool fullyLinked = false;

    void lock();
    void unlock();

    std::shared_ptr<Node<T>> getNext(int level);

    int topLevel;
    bool isTail = false;

   private:
    T start;
    T end;
    int level;
    mutable std::recursive_mutex mutex;
};

template <typename T>
Node<T>::Node(T start, T end, int level) : start{start}, end{end}, fullyLinked{false}, topLevel{level} {
    next.resize(level + 1);
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
T Node<T>::getStart() const {
    return start;
}

template <typename T>
T Node<T>::getEnd() const {
    return end;
}

template <typename T>
void Node<T>::setStart(T newStart) {
    start = newStart;
}

template <typename T>
void Node<T>::setEnd(T newEnd) {
    end = newEnd;
}

template <typename T>
std::shared_ptr<Node<T>> Node<T>::getNext(int level) {
    if (level >= next.size() || level < 0) {
        std::cerr << "Access out of range!" << std::endl;
        exit(0);
    }

    return next.at(level);
}

template <typename T>
void Node<T>::info() {
    std::cout << "Thread " << std::this_thread::get_id() << ". Node start " << start << " end " << end << " next " << next.size() << " marked: " << marked << std::endl;
}

