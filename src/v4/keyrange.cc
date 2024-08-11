#include "keyrange.h"

// create a keyrange by borrowing the left and right dbt
// pointers. no memory is copied. no checks for infinity needed.
void keyrange::create(uint64_t left, uint64_t right) {
    m_left_key = left;
    m_right_key = right;
}

// compare ranges.
keyrange::comparison keyrange::compare(const keyrange &range) const {
    if (m_right_key < range.m_left_key) {
        return comparison::LESS_THAN;
    } else if (m_left_key > range.m_right_key) {
        return comparison::GREATER_THAN;
    } else if ((m_right_key == m_right_key) &&
               (m_left_key == range.m_left_key)) {
        return comparison::EQUALS;
    } else {
        return comparison::OVERLAPS;
    }
}

// equality is a stronger form of overlapping.
// so two ranges "overlap" if they're either equal or just overlapping.
bool keyrange::overlaps(const keyrange &range) const {
    comparison c = compare(range);
    return c == comparison::EQUALS || c == comparison::OVERLAPS;
}

keyrange keyrange::get_infinite_range(void) {
    keyrange range;
    range.create(UINT64_MAX, UINT64_MAX);
    return range;
}

void keyrange::create_copy(const keyrange &range) {
    m_left_key = range.m_left_key;
    m_right_key = range.m_right_key;
}
