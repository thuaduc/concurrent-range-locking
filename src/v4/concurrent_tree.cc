#include "concurrent_tree.h"

void concurrent_tree::create() {
    // start with an empty root node. we do this instead of
    // setting m_root to null so there's always a root to lock
    m_root.create_root();
}

void concurrent_tree::destroy(void) { m_root.destroy_root(); }

bool concurrent_tree::is_empty(void) { return m_root.is_empty(); }

uint64_t concurrent_tree::get_insertion_memory_overhead(void) {
    return sizeof(treenode);
}

void concurrent_tree::locked_keyrange::prepare(concurrent_tree *tree) {
    // the first step in acquiring a locked keyrange is locking the root
    treenode *const root = &tree->m_root;
    m_tree = tree;
    m_subtree = root;
    m_range = keyrange::get_infinite_range();
    root->mutex_lock();
}

void concurrent_tree::locked_keyrange::acquire(uint64_t left, uint64_t right) {
    treenode *const root = &m_tree->m_root;

    keyrange range;
    range.create(left, right);

    treenode *subtree;
    if (root->is_empty() || root->range_overlaps(range)) {
        subtree = root;
    } else {
        const keyrange::comparison *cmp_hint = nullptr;
        subtree = root->find_node_with_overlapping_child(range, cmp_hint);
    }

    // invariant_notnull(subtree);
    m_range = range;
    m_subtree = subtree;
}

void concurrent_tree::locked_keyrange::release(void) {
    m_subtree->mutex_unlock();
}

void concurrent_tree::locked_keyrange::insert(const keyrange &range) {
    // empty means no children, and only the root should ever be empty
    if (m_subtree->is_empty()) {
        m_subtree->set_range_and_txnid(range);
    } else {
        m_subtree->insert(range);
    }
}

void concurrent_tree::locked_keyrange::remove(const keyrange &range) {
    // invariant(!m_subtree->is_empty());
    treenode *new_subtree = m_subtree->remove(range);

    // if removing range changed the root of the subtree,
    // then the subtree must be the root of the entire tree.
    if (new_subtree == nullptr) {
        // invariant(m_subtree->is_root());
        // invariant(m_subtree->is_empty());
    }
}
