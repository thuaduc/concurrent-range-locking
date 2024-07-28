#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

/*
An AtomicMarkableReference<T> is an object from the java.util.concurrent.atomic
package that encapsulates both a reference to an object of type T and a Boolean
mark. These fields can be updated atomically, either together or individually.

––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

compareAndSet() method tests the expected reference and mark values, and if both
tests succeed, replaces them with updated reference and mark values.

boolean compareAndSet(T expectedReference, T newReference, boolean
expectedMark, boolean newMark);

––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

attemptMark() method tests an expected reference value and if the test succeeds,
replaces it with a new mark value.

boolean attemptMark(T expectedReference, boolean newMark);

––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

The get() method returns the object’s reference value and stores the mark value
in a Boolean array argument.

T get(boolean[] marked);
*/

static const uintptr_t MARK_MASK = 0x1;

template <typename T>
class AtomicMarkableReference {
   private:
    std::atomic<uintptr_t> atomicRefMark;

    uintptr_t pack(T *ref, bool mark) const;
    std::pair<T *, bool> unpack(uintptr_t packed) const;

   public:
    AtomicMarkableReference();

    AtomicMarkableReference(const AtomicMarkableReference &) = delete;
    AtomicMarkableReference &operator=(const AtomicMarkableReference &) =
        delete;
    AtomicMarkableReference(AtomicMarkableReference &&other) noexcept;
    AtomicMarkableReference &operator=(
        AtomicMarkableReference &&other) noexcept;

    void store(T *ref, bool mark);
    bool compareAndSet(T *expectedRef, T *newRef, bool expectedMark,
                       bool newMark);
    bool attemptMark(T *expectedRef, bool newMark);

    T *get(bool *mark) const;
    T *getReference() const;
};

template <typename T>
uintptr_t AtomicMarkableReference<T>::pack(T *ref, bool mark) const {
    return reinterpret_cast<uintptr_t>(ref) | (mark ? 1 : 0);
}

template <typename T>
std::pair<T *, bool> AtomicMarkableReference<T>::unpack(
    uintptr_t packed) const {
    T *ref = reinterpret_cast<T *>(packed & ~MARK_MASK);
    bool mark = packed & MARK_MASK;
    return {ref, mark};
}

template <typename T>
AtomicMarkableReference<T>::AtomicMarkableReference() {
    atomicRefMark.store(pack(nullptr, false), std::memory_order_relaxed);
}

template <typename T>
AtomicMarkableReference<T>::AtomicMarkableReference(
    AtomicMarkableReference &&other) noexcept {
    atomicRefMark.store(other.atomicRefMark.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
}

template <typename T>
AtomicMarkableReference<T> &AtomicMarkableReference<T>::operator=(
    AtomicMarkableReference &&other) noexcept {
    if (this != &other) {
        atomicRefMark.store(other.atomicRefMark.load(std::memory_order_relaxed),
                            std::memory_order_relaxed);
    }
    return *this;
}

template <typename T>
void AtomicMarkableReference<T>::store(T *ref, bool mark) {
    atomicRefMark.store(pack(ref, mark), std::memory_order_relaxed);
}

template <typename T>
bool AtomicMarkableReference<T>::compareAndSet(T *expectedRef, T *newRef,
                                               bool expectedMark,
                                               bool newMark) {
    uintptr_t expected = pack(expectedRef, expectedMark);
    uintptr_t desired = pack(newRef, newMark);
    return atomicRefMark.compare_exchange_strong(expected, desired,
                                                 std::memory_order_acq_rel);
}

template <typename T>
bool AtomicMarkableReference<T>::attemptMark(T *expectedRef, bool newMark) {
    uintptr_t current = atomicRefMark.load(std::memory_order_acquire);
    auto [currentRef, currentMark] = unpack(current);
    if (currentRef == expectedRef && currentMark != newMark) {
        uintptr_t desired = pack(expectedRef, newMark);
        return atomicRefMark.compare_exchange_strong(current, desired,
                                                     std::memory_order_acq_rel);
    }
    return false;
}

template <typename T>
T *AtomicMarkableReference<T>::get(bool *mark) const {
    auto [ref, currentMark] =
        unpack(atomicRefMark.load(std::memory_order_acquire));
    mark[0] = currentMark;
    return ref;
}

template <typename T>
T *AtomicMarkableReference<T>::getReference() const {
    auto [ref, _] = unpack(atomicRefMark.load(std::memory_order_acquire));
    return ref;
}