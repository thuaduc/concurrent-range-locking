#pragma once

#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <vector>

class SkipListNode {
   public:
    uint64_t start;
    uint64_t end;
    uint8_t level;
    SkipListNode** forward;

    SkipListNode(uint64_t start, uint64_t end, uint8_t level)
        : start(start), end(end), level(level) {
        forward = (SkipListNode**)malloc(sizeof(SkipListNode*) * (level + 1));
        for (int i = 0; i <= level; ++i) {
            forward[i] = nullptr;
        }
    }

    ~SkipListNode() { free(forward); }
};

class SongRangeLock {
   public:
    static constexpr uint8_t MAX_LEVEL = 3;
    static constexpr uint64_t MAX_VALUE = ~0ULL;

    explicit SongRangeLock();
    ~SongRangeLock();

    bool tryLock(uint64_t start, uint64_t end);
    void releaseLock(uint64_t start);

    size_t size();
    void displayList();

    SkipListNode* head_;
    SkipListNode* tail_;

   private:
    SkipListNode* AllocNode(uint64_t start, uint64_t end, uint8_t level);
    bool FindNodes(uint64_t start, uint64_t end, SkipListNode** out_nodes);
    void InsertRange(SkipListNode** nodes, uint64_t start, uint64_t end);
    int randomLevel();

    std::mutex spinlock_;
    std::atomic<size_t> elementsCount{0};
};