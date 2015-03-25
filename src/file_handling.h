#pragma once
#include <ostream>
#include <stdint.h>

typedef int64_t PositionIndex;

struct PositionRange {
    PositionIndex start;
    PositionIndex end;

    PositionRange(int64_t start, int64_t end) {
        this->start = start;
        this->end = end;
    }
};

std::ostream& operator<<(std::ostream &out, const PositionRange &range);
