#include "treenode.h"

void treenode::mutex_lock(void) { m_mutex.lock(); }

void treenode::mutex_unlock(void) { m_mutex.unlock(); }

void treenode::init() {
    m_is_root = false;
    m_is_empty = true;
    m_left_child.set(nullptr);
    m_right_child.set(nullptr);
}

void treenode::create_root() {
    init();
    m_is_root = true;
}

void treenode::destroy_root(void) {
    // invariant(is_root());
    // invariant(is_empty());
}

void treenode::set_range_and_txnid(const keyrange &range) {
    // allocates a new copy of the range for this node
    // m_range.create_copy(range);
    m_range.create_copy(range);
    m_is_empty = false;
}

bool treenode::is_root(void) { return m_is_root; }

bool treenode::is_empty(void) { return m_is_empty; }

bool treenode::range_overlaps(const keyrange &range) {
    return m_range.overlaps(range);
}

treenode *treenode::alloc(const keyrange &range) {
    treenode *node = new treenode();
    node->init();
    node->set_range_and_txnid(range);
    return node;
}

void treenode::free(treenode *node) {
    // the root is simply marked as empty.
    if (node->is_root()) {
        node->m_is_empty = true;
    }
}

uint32_t treenode::get_depth_estimate(void) const {
    const uint32_t left_est = m_left_child.depth_est;
    const uint32_t right_est = m_right_child.depth_est;
    return (left_est > right_est ? left_est : right_est) + 1;
}

treenode *treenode::find_node_with_overlapping_child(
    const keyrange &range, const keyrange::comparison *cmp_hint) {
    // determine which child to look at based on a comparison. if we were
    // given a comparison hint, use that. otherwise, compare them now.
    keyrange::comparison c = cmp_hint ? *cmp_hint : range.compare(m_range);

    treenode *child;
    if (c == keyrange::comparison::LESS_THAN) {
        child = lock_and_rebalance_left();
    } else {
        // The caller (locked_keyrange::acquire) handles the case where
        // the root of the locked_keyrange is the node that overlaps.
        // range is guaranteed not to overlap this node.
        // invariant(c == keyrange::comparison::GREATER_THAN);
        child = lock_and_rebalance_right();
    }

    // if the search would lead us to an empty subtree (child == nullptr),
    // or the child overlaps, then we know this node is the parent we want.
    // otherwise we need to recur into that child.
    if (child == nullptr) {
        return this;
    } else {
        c = range.compare(child->m_range);
        if (c == keyrange::comparison::EQUALS ||
            c == keyrange::comparison::OVERLAPS) {
            child->mutex_unlock();
            return this;
        } else {
            // unlock this node before recurring into the locked child,
            // passing in a comparison hint since we just comapred range
            // to the child's range.
            mutex_unlock();
            return child->find_node_with_overlapping_child(range, &c);
        }
    }
}

void treenode::insert(const keyrange &range) {
    // choose a child to check. if that child is null, then insert the new
    // node there. otherwise recur down that child's subtree
    keyrange::comparison c = range.compare(m_range);
    if (c == keyrange::comparison::LESS_THAN) {
        treenode *left_child = lock_and_rebalance_left();
        if (left_child == nullptr) {
            left_child = treenode::alloc(range);
            m_left_child.set(left_child);
        } else {
            left_child->insert(range);
            left_child->mutex_unlock();
        }
    } else {
        // invariant(c == keyrange::comparison::GREATER_THAN);
        treenode *right_child = lock_and_rebalance_right();
        if (right_child == nullptr) {
            right_child = treenode::alloc(range);
            m_right_child.set(right_child);
        } else {
            right_child->insert(range);
            right_child->mutex_unlock();
        }
    }
}

treenode *treenode::find_child_at_extreme(int direction, treenode **parent) {
    treenode *child =
        direction > 0 ? m_right_child.get_locked() : m_left_child.get_locked();

    if (child) {
        *parent = this;
        treenode *child_extreme =
            child->find_child_at_extreme(direction, parent);
        child->mutex_unlock();
        return child_extreme;
    } else {
        return this;
    }
}

treenode *treenode::find_leftmost_child(treenode **parent) {
    return find_child_at_extreme(-1, parent);
}

treenode *treenode::find_rightmost_child(treenode **parent) {
    return find_child_at_extreme(1, parent);
}

treenode *treenode::remove_root_of_subtree() {
    // if this node has no children, just free it and return null
    if (m_left_child.ptr == nullptr && m_right_child.ptr == nullptr) {
        // treenode::free requires that non-root nodes are unlocked
        if (!is_root()) {
            mutex_unlock();
        }
        treenode::free(this);
        return nullptr;
    }

    // we have a child, so get either the in-order successor or
    // predecessor of this node to be our replacement.
    // replacement_parent is updated by the find functions as
    // they recur down the tree, so initialize it to this.
    treenode *child, *replacement;
    treenode *replacement_parent = this;
    if (m_left_child.ptr != nullptr) {
        child = m_left_child.get_locked();
        replacement = child->find_rightmost_child(&replacement_parent);
        // invariant(replacement == child || replacement_parent != this);

        // detach the replacement from its parent
        if (replacement_parent == this) {
            m_left_child = replacement->m_left_child;
        } else {
            replacement_parent->m_right_child = replacement->m_left_child;
        }
    } else {
        child = m_right_child.get_locked();
        replacement = child->find_leftmost_child(&replacement_parent);
        // invariant(replacement == child || replacement_parent != this);

        // detach the replacement from its parent
        if (replacement_parent == this) {
            m_right_child = replacement->m_right_child;
        } else {
            replacement_parent->m_left_child = replacement->m_right_child;
        }
    }
    child->mutex_unlock();

    // swap in place with the detached replacement, then destroy it
    treenode::swap_in_place(replacement, this);
    treenode::free(replacement);

    return this;
}

treenode *treenode::remove(const keyrange &range) {
    treenode *child;
    // if the range is equal to this node's range, then just remove
    // the root of this subtree. otherwise search down the tree
    // in either the left or right children.
    keyrange::comparison c = range.compare(m_range);
    switch (c) {
        case keyrange::comparison::EQUALS:
            return remove_root_of_subtree();
        case keyrange::comparison::LESS_THAN:
            child = m_left_child.get_locked();
            // invariant_notnull(child);
            child = child->remove(range);

            // unlock the child if there still is one.
            // regardless, set the right child pointer
            if (child) {
                child->mutex_unlock();
            }
            m_left_child.set(child);
            break;
        case keyrange::comparison::GREATER_THAN:
            child = m_right_child.get_locked();
            // invariant_notnull(child);
            child = child->remove(range);

            // unlock the child if there still is one.
            // regardless, set the right child pointer
            if (child) {
                child->mutex_unlock();
            }
            m_right_child.set(child);
            break;
        case keyrange::comparison::OVERLAPS:
            // shouldn't be overlapping, since the tree is
            // non-overlapping and this range must exist
            abort();
    }

    return this;
}

bool treenode::left_imbalanced(int threshold) const {
    uint32_t left_depth = m_left_child.depth_est;
    uint32_t right_depth = m_right_child.depth_est;
    return m_left_child.ptr != nullptr && left_depth > threshold + right_depth;
}

bool treenode::right_imbalanced(int threshold) const {
    uint32_t left_depth = m_left_child.depth_est;
    uint32_t right_depth = m_right_child.depth_est;
    return m_right_child.ptr != nullptr && right_depth > threshold + left_depth;
}

// effect: rebalances the subtree rooted at this node
//         using AVL style O(1) rotations. unlocks this
//         node if it is not the new root of the subtree.
// requires: node is locked by this thread, children are not
// returns: locked root node of the rebalanced tree
treenode *treenode::maybe_rebalance(void) {
    // if we end up not rotating at all, the new root is this
    treenode *new_root = this;
    treenode *child = nullptr;

    if (left_imbalanced(IMBALANCE_THRESHOLD)) {
        child = m_left_child.get_locked();
        if (child->right_imbalanced(0)) {
            treenode *grandchild = child->m_right_child.get_locked();

            child->m_right_child = grandchild->m_left_child;
            grandchild->m_left_child.set(child);

            m_left_child = grandchild->m_right_child;
            grandchild->m_right_child.set(this);

            new_root = grandchild;
        } else {
            m_left_child = child->m_right_child;
            child->m_right_child.set(this);
            new_root = child;
        }
    } else if (right_imbalanced(IMBALANCE_THRESHOLD)) {
        child = m_right_child.get_locked();
        if (child->left_imbalanced(0)) {
            treenode *grandchild = child->m_left_child.get_locked();

            child->m_left_child = grandchild->m_right_child;
            grandchild->m_right_child.set(child);

            m_right_child = grandchild->m_left_child;
            grandchild->m_left_child.set(this);

            new_root = grandchild;
        } else {
            m_right_child = child->m_left_child;
            child->m_left_child.set(this);
            new_root = child;
        }
    }

    // up to three nodes may be locked.
    // - this
    // - child
    // - grandchild (but if it is locked, its the new root)
    //
    // one of them is the new root. we unlock everything except the new
    // root.
    if (child && child != new_root) {
        child->mutex_unlock();
    }
    if (this != new_root) {
        mutex_unlock();
    }
    return new_root;
}

treenode *treenode::lock_and_rebalance_left(void) {
    treenode *child = m_left_child.get_locked();
    if (child) {
        treenode *new_root = child->maybe_rebalance();
        m_left_child.set(new_root);
        child = new_root;
    }
    return child;
}

treenode *treenode::lock_and_rebalance_right(void) {
    treenode *child = m_right_child.get_locked();
    if (child) {
        treenode *new_root = child->maybe_rebalance();
        m_right_child.set(new_root);
        child = new_root;
    }
    return child;
}

void treenode::child_ptr::set(treenode *node) {
    ptr = node;
    depth_est = ptr ? ptr->get_depth_estimate() : 0;
}

treenode *treenode::child_ptr::get_locked(void) {
    if (ptr) {
        ptr->mutex_lock();
        depth_est = ptr->get_depth_estimate();
    }
    return ptr;
}

void treenode::swap_in_place(treenode *node1, treenode *node2) {
    keyrange tmp_range = node1->m_range;
    node1->m_range = node2->m_range;
    node2->m_range = tmp_range;
}