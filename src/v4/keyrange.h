#pragma once
#include <cstdint>

class keyrange {
   public:
    enum comparison { EQUALS, LESS_THAN, GREATER_THAN, OVERLAPS };

    // effect: compares this range to the given range
    // returns: LESS_THAN    if given range is strictly to the left
    //          GREATER_THAN if given range is strictly to the right
    //          EQUALS       if given range has the same left and right
    //          endpoints OVERLAPS     if at least one of the given range's
    //          endpoints falls
    //                       between this range's endpoints
    comparison compare(const keyrange &range) const;

    void create(uint64_t left, uint64_t right);
    void create_copy(const keyrange &range);
    bool overlaps(const keyrange &range) const;

    static keyrange get_infinite_range(void);

    uint64_t m_left_key;
    uint64_t m_right_key;
};
