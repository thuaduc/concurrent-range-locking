#pragma once
#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

template <typename T, typename K>
struct Node {
    Node(T key, K value, int level);
    ~Node() = default;

    T getKey() const;
    K getValue() const;
    void setKey(T newKey);
    void setValue(K newValue);

    std::vector<std::shared_ptr<Node>> forward;
    bool marked;
    bool fullyLinked;

    void lock();
    void unlock();

   private:
    T key;
    K value;
    int level;
    mutable std::recursive_mutex mutex;
};

template <typename T, typename K>
Node<T, K>::Node(T key, K value, int level) : key{key}, value{value} {
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
T Node<T, K>::getKey() const {
    return key;
}

template <typename T, typename K>
K Node<T, K>::getValue() const {
    return value;
}

template <typename T, typename K>
void Node<T, K>::setKey(T newKey) {
    key = newKey;
}

template <typename T, typename K>
void Node<T, K>::setValue(K newValue) {
    value = newValue;
}
