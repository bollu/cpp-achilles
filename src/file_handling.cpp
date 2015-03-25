#include "file_handling.h"
#include <iostream>

std::ostream& operator<<(std::ostream &out, const PositionRange &range) {
    out<<"("<<range.start<<":"<<range.end<<")";
    return out;
}


