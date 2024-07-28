#include <functional>
#include <memory>
#include <utility>

#include "./atomic_reference.hpp"

template <typename T>
class Node {
   public:
    T start;
    T end;
    int topLevel;

    Node();
    ~Node();

    void initialize(T start, T end, int topLevel);
    void initializeHead(T start, T end, int topLevel, Node<T>* tail);

    int getTopLevel() const;
    T getStart() const;
    T getEnd() const;

    AtomicMarkableReference<Node<T>>** next;
};

template <typename T>
Node<T>::Node() {}

template <typename T>
void Node<T>::initialize(T start, T end, int topLevel) {
    this->start = start;
    this->end = end;
    this->topLevel = topLevel;

    next = new AtomicMarkableReference<Node<T>>*[topLevel + 1];
    for (int i = 0; i <= topLevel; ++i) {
        next[i] = new AtomicMarkableReference<Node<T>>();
    }
}

template <typename T>
void Node<T>::initializeHead(T start, T end, int topLevel, Node<T>* tail) {
    this->start = start;
    this->end = end;
    this->topLevel = topLevel;

    next = new AtomicMarkableReference<Node<T>>*[topLevel + 1];
    for (int i = 0; i <= topLevel; ++i) {
        next[i] = new AtomicMarkableReference<Node<T>>();
        next[i]->store(tail, false);
    }
}

template <typename T>
Node<T>::~Node() {
    // delete
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