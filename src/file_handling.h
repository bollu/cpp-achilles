#pragma once
#include <ostream>
#include <stdint.h>
#include <assert.h>

typedef int64_t PositionIndex;

struct PositionRange {
    PositionIndex start;
    PositionIndex end;
    std::string &file_data;

    PositionRange(PositionIndex start, PositionIndex end, std::string &file_data) :
        file_data(file_data), start(start), end(end){
            assert(start <= end);
        }

    PositionRange extend_end(PositionRange extended);
    PositionRange& operator =(const PositionRange &other); 
};

std::ostream& operator<<(std::ostream &out, const PositionRange &range);
