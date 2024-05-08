#pragma once
#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

template <typename T, typename K>
struct Node {
    Node(T start, K end, int level);
    ~Node() = default;

    T getStart() const;
    K getEnd() const;
    void setStart(T newStart);
    void setEnd(K newEnd);

    std::vector<std::shared_ptr<Node>> forward;
    bool marked;
    bool fullyLinked;

    void lock();
    void unlock();

   private:
    T start;
    K end;
    int level;
    mutable std::recursive_mutex mutex;
};

template <typename T, typename K>
Node<T, K>::Node(T start, K end, int level) : start{start}, end{end} {
    forward.resize(level + 1);
}

template <typename T, typename K>
void Node<T, K>::lock() {
    mutex.lock();
}

template <typename T, typename K>
void Node<T, K>::unlock() {
    mutex.unlock();
}

template <typename T, typename K>
T Node<T, K>::getStart() const {
    return start;
}

template <typename T, typename K>
K Node<T, K>::getEnd() const {
    return end;
}

template <typename T, typename K>
void Node<T, K>::setStart(T newStart) {
    start = newStart;
}

template <typename T, typename K>
void Node<T, K>::setEnd(K newEnd) {
    end = newEnd;
}
