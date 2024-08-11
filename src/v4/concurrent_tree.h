#include "keyrange.h"
#include "treenode.h"

class concurrent_tree {
   public:
    // A locked_keyrange gives you exclusive access to read and write
    // operations that occur on any keys in that range. You only have
    // the right to operate on keys in that range or keys that were read
    // from the keyrange using iterate()
    //
    // Access model:
    // - user prepares a locked keyrange. all threads serialize behind
    // prepare().
    // - user breaks the serialzation point by acquiring a range, or releasing.
    // - one thread operates on a certain locked_keyrange object at a time.
    // - when the thread is finished, it releases

    class locked_keyrange {
       public:
        // effect: prepare to acquire a locked keyrange over the given
        //         concurrent_tree, preventing other threads from preparing
        //         until this thread either does acquire() or release().
        // note: operations performed on a prepared keyrange are equivalent
        //         to ones performed on an acquired keyrange over -inf, +inf.
        // rationale: this provides the user with a serialization point for
        // descending
        //            or modifying the the tree. it also proives a convenient
        //            way of doing serializable operations on the tree.
        // There are two valid sequences of calls:
        //  - prepare, acquire, [operations], release
        //  - prepare, [operations],release
        void prepare(concurrent_tree *tree);

        // requires: the locked keyrange was prepare()'d
        // effect: acquire a locked keyrange over the given concurrent_tree.
        //         the locked keyrange represents the range of keys overlapped
        //         by the given range
        void acquire(uint64_t left, uint64_t right);

        // effect: releases a locked keyrange and the mutex it holds
        void release(void);

        // inserts the given range into the tree, with an associated txnid.
        // requires: range does not overlap with anything in this
        // locked_keyrange rationale: caller is responsible for only inserting
        // unique ranges
        void insert(const keyrange &range);

        // effect: removes the given range from the tree
        // requires: range exists exactly in this locked_keyrange
        // rationale: caller is responsible for only removing existing ranges
        void remove(const keyrange &range);

       private:
        // the concurrent tree this locked keyrange is for
        concurrent_tree *m_tree;

        // the range of keys this locked keyrange represents
        keyrange m_range;

        // the subtree under which all overlapping ranges exist
        treenode *m_subtree;
    };

    // effect: initialize the tree to an empty state
    void create();

    // effect: destroy the tree.
    // requires: tree is empty
    void destroy(void);

    // returns: true if the tree is empty
    bool is_empty(void);

    // returns: the memory overhead of a single insertion into the tree
    static uint64_t get_insertion_memory_overhead(void);

   private:
    // the root needs to always exist so there's a lock to grab
    // even if the tree is empty. that's why we store a treenode
    // here and not a pointer to one.
    treenode m_root;
};
