#include "range_lock.hpp"

#include <algorithm>

SongRangeLock::SongRangeLock()
    : head_(AllocNode(0, 0, MAX_LEVEL)),
      tail_(AllocNode(MAX_VALUE, MAX_VALUE, MAX_LEVEL)) {
    for (auto i = 0; i <= MAX_LEVEL; ++i) {
        head_->forward[i] = tail_;
    }
}

SongRangeLock::~SongRangeLock() {
    auto p = head_;
    do {
        auto q = p->forward[0];
        free(p);
        p = q;
    } while (p != nullptr);
}

SkipListNode *SongRangeLock::AllocNode(uint64_t start, uint64_t end,
                                   uint8_t level) {
    return new SkipListNode(start, end, level);
}

bool SongRangeLock::FindNodes(uint64_t start, uint64_t end,
                          SkipListNode **out_nodes) {
    SkipListNode *curr;
    auto pred = head_;
    for (int k = MAX_LEVEL; k >= 0; k--) {
        while (curr = pred->forward[k], curr->end < start) {
            pred = curr;
        }
        out_nodes[k] = pred;
    }

    if (pred == head_) {
        return !(start >= pred->end && end < curr->start);
    } else {
        return !(start > pred->end && end < curr->start);
    }
}

void SongRangeLock::InsertRange(SkipListNode **nodes, uint64_t start,
                            uint64_t end) {
    auto lv = randomLevel();

    /* Insert start and end */
    auto q = AllocNode(start, end, lv);  //
    for (auto k = 0; k <= lv; k++) {
        auto p = nodes[k];
        q->forward[k] = p->forward[k];
        p->forward[k] = q;
    }
}

bool SongRangeLock::tryLock(uint64_t start, uint64_t end) {
    std::lock_guard<std::mutex> lock(spinlock_);
    SkipListNode *nodes[MAX_LEVEL + 1];

    if (FindNodes(start, end, nodes)) {
        return false;
    }

    InsertRange(nodes, start, end);
    elementsCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void SongRangeLock::releaseLock(uint64_t start) {
    std::lock_guard<std::mutex> lock(spinlock_);

    SkipListNode *curr;
    SkipListNode *preds[MAX_LEVEL + 1];
    SkipListNode *succ;

    auto pred = head_;
    for (int level = static_cast<int>(MAX_LEVEL); level >= 0; level--) {
        while ((succ = pred->forward[level]) && succ->start < start) {
            pred = succ;
        }
        preds[level] = pred;
    }
    curr = preds[0]->forward[0];

    for (int level = 0;
         level <= MAX_LEVEL && (pred = preds[level])->forward[level] == curr;
         level++) {
        pred->forward[level] = curr->forward[level];
    }

    elementsCount.fetch_sub(1, std::memory_order_relaxed);
}

void SongRangeLock::displayList() {
    std::cout << "Concurrent Range Lock" << std::endl;

    if (elementsCount == 0) {
        std::cout << "List is empty" << std::endl;
        return;
    }

    int len = static_cast<int>(elementsCount);

    std::vector<std::vector<std::string>> builder(
        len, std::vector<std::string>(MAX_LEVEL + 1));

    auto *current = head_->forward[0];

    for (int i = 0; i < len; ++i) {
        for (int j = 0; j <= MAX_LEVEL; ++j) {
            if (j <= current->level) {
                std::ostringstream oss;
                oss << "[" << std::setw(2) << std::setfill('0')
                    << current->start << "," << std::setw(2)
                    << std::setfill('0') << current->end << "]";
                builder[i][j] = oss.str();
            } else {
                builder[i][j] = "---------";
            }
        }
        current = current->forward[0];
    }

    for (int i = MAX_LEVEL; i >= 0; --i) {
        std::cout << "Level " << i << ": head ";
        for (int j = 0; j < len; ++j) {
            if (builder[j][i] == "---------") {
                std::cout << "---------";
            } else {
                std::cout << "->" << builder[j][i];
            }
        }
        std::cout << "---> tail" << std::endl;
    }
}

size_t SongRangeLock::size() { return elementsCount.load(); }

int SongRangeLock::randomLevel() {
    int level = 0;
    while (rand() % 2 && level < MAX_LEVEL) {
        level++;
    }
    return level;
}