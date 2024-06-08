#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include <cstdlib>
#include <functional>

class ScopeGuard
{
public:
    explicit ScopeGuard(std::function<void()> onExitScope)
        : onExitScope_(onExitScope), dismissed_(false) {}

    ~ScopeGuard()
    {
        if (!dismissed_)
        {
            onExitScope_();
        }
    }

    void Dismiss()
    {
        dismissed_ = true;
    }

private:
    std::function<void()> onExitScope_;
    bool dismissed_;
};

template <typename T>
struct Node {
    Node(T start, T end, int level);
    ~Node();

    int getTopLevel() const;
    T getStart() const;
    T getEnd() const;

    Node **next;
    Node *removed = nullptr;
    bool marked = false;
    bool fullyLinked = false;

    void lock();
    void unlock();

private:
    T start;
    T end;
    int topLevel;
    mutable std::recursive_mutex mutex;
};

template <typename T>
Node<T>::Node(T start, T end, int level) : start{start}, end{end}, topLevel{level} {
    next = new Node<T>*[level + 1];
}

template <typename T>
Node<T>::~Node(){
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