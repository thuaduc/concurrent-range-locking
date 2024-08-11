#pragma once

#include <mutex>
#include <string>

#include "keyrange.h"

class treenode {
   public:
    // every treenode function has some common requirements:
    // - node is locked and children are never locked
    // - node may be unlocked if no other thread has visibility

    void create_root();

    void destroy_root(void);

    // effect: sets the txnid and copies the given range for this node
    void set_range_and_txnid(const keyrange &range);

    // returns: true iff this node is marked as empty
    bool is_empty(void);

    // returns: true if this is the root node, denoted by a null parent
    bool is_root(void);

    // returns: true if the given range overlaps with this node's range
    bool range_overlaps(const keyrange &range);

    // effect: locks the node
    void mutex_lock(void);

    // effect: unlocks the node
    void mutex_unlock(void);

    // return: node whose child overlaps, or a child that is empty
    //         and would contain range if it existed
    // given: if cmp_hint is non-null, then it is a precomputed
    //        comparison of this node's range to the given range.
    treenode *find_node_with_overlapping_child(
        const keyrange &range, const keyrange::comparison *cmp_hint);

    // effect: inserts the given range and txnid into a subtree, recursively
    // requires: range does not overlap with any node below the subtree
    void insert(const keyrange &range);

    // effect: removes the given range from the subtree
    // requires: range exists in the subtree
    // returns: the root of the resulting subtree
    treenode *remove(const keyrange &range);

   private:
    // the child_ptr is a light abstraction for the locking of
    // a child and the maintenence of its depth estimate.

    struct child_ptr {
        // set the child pointer
        void set(treenode *node);

        // get and lock this child if it exists
        treenode *get_locked(void);

        treenode *ptr;
        uint32_t depth_est;
    };

    // the balance factor at which a node is considered imbalanced
    static const int32_t IMBALANCE_THRESHOLD = 2;

    // node-level mutex
    std::mutex m_mutex;

    keyrange m_range;

    // two child pointers
    child_ptr m_left_child;
    child_ptr m_right_child;

    // marked for the root node. the root node is never free()'d
    // when removed, but instead marked as empty.
    bool m_is_root;

    // marked for an empty node. only valid for the root.
    bool m_is_empty;

    // effect: initializes an empty node with the given comparator
    void init();

    // requires: *parent is initialized to something meaningful.
    // requires: subtree is non-empty
    // returns: the leftmost child of the given subtree
    // returns: a pointer to the parent of said child in *parent, only
    //          if this function recurred, otherwise it is untouched.
    treenode *find_leftmost_child(treenode **parent);

    // requires: *parent is initialized to something meaningful.
    // requires: subtree is non-empty
    // returns: the rightmost child of the given subtree
    // returns: a pointer to the parent of said child in *parent, only
    //          if this function recurred, otherwise it is untouched.
    treenode *find_rightmost_child(treenode **parent);

    // effect: remove the root of this subtree, destroying the old root
    // returns: the new root of the subtree
    treenode *remove_root_of_subtree(void);

    // requires: subtree is non-empty, direction is not 0
    // returns: the child of the subtree at either the left or rightmost extreme
    treenode *find_child_at_extreme(int direction, treenode **parent);

    // effect: retrieves and possibly rebalances the left child
    // returns: a locked left child, if it exists
    treenode *lock_and_rebalance_left(void);

    // effect: retrieves and possibly rebalances the right child
    // returns: a locked right child, if it exists
    treenode *lock_and_rebalance_right(void);

    // returns: the estimated depth of this subtree
    uint32_t get_depth_estimate(void) const;

    // returns: true iff left subtree depth is sufficiently less than the right
    bool left_imbalanced(int threshold) const;

    // returns: true iff right subtree depth is sufficiently greater than the
    // left
    bool right_imbalanced(int threshold) const;

    // effect: performs an O(1) rebalance, which will "heal" an imbalance by at
    // most 1. effect: if the new root is not this node, then this node is
    // unlocked. returns: locked node representing the new root of the
    // rebalanced subtree
    treenode *maybe_rebalance(void);

    // returns: allocated treenode populated with a copy of the range and txnid
    static treenode *alloc(const keyrange &range);

    // requires: node is a locked root node, or an unlocked non-root node
    static void free(treenode *node);

    // effect: swaps the range/txnid pairs for node1 and node2.
    static void swap_in_place(treenode *node1, treenode *node2);
};
