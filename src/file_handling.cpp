#include "file_handling.h"
#include <iostream>
#include <tuple>
#include <algorithm>
#include <sstream>

std::pair<PositionIndex, PositionIndex>index_to_line_col(
    const PositionIndex& idx,
    std::string        & file_data) {
    PositionIndex line = 1;
    PositionIndex col  = 0;

    PositionIndex current_index = 0;

    for (auto ch : file_data) {
        if (current_index == idx) {
            break;
        }

        if (ch == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }

        current_index++;
    }

    return std::make_pair(line, col);
}

PositionRange& PositionRange::operator=(const PositionRange& other) {
    this->file_data = other.file_data;
    this->start     = other.start;
    this->end       = other.end;

    return *this;
}

PositionRange PositionRange::extend_end(PositionRange extended) {
    assert(this->end <= extended.start);
    return PositionRange(this->start, extended.end, this->file_data);
}

std::string get_line(std::string& file_data, int line_number) {
    int begin_index         = 0;
    int current_line_number = 1;

    while (true) {
        if (line_number == current_line_number) {
            break;
        }

        if (file_data[begin_index] == '\n') {
            current_line_number++;
        }

        begin_index++;
    }

    int end_index = begin_index;

    while (file_data[end_index] != '\n') {
        end_index++;
    }

    // tabs will screw pretty printing up (since tab with is unknown)
    std::string raw_str =
        file_data.substr(begin_index, end_index - begin_index);
    std::replace(raw_str.begin(), raw_str.end(), '\t', ' ');
    return raw_str;
}

std::ostream& operator<<(std::ostream& out, const PositionRange& range) {
    auto start_line_col = index_to_line_col(range.start, range.file_data);
    auto end_line_col   = index_to_line_col(range.end, range.file_data);

    out << "\nposition: ";
    out << "(" << start_line_col.first << ":" << start_line_col.second << ")";
    out << " to ";
    out << "(" << end_line_col.first << ":" << end_line_col.second << ")";

    out << "\n";

    // if they're of the type (2:8 ) to (3:0) then make them both on the same
    // line

    /*
       if (start_line_col.first + 1 == end_line_col.first && end_line_col.second
          == 0) {
        PositionRange new_end_range = range;
        new_end_range.end -= 1;
        end_line_col  = index_to_line_col(new_end_range.end, range.file_data);
       };
     */

    // same line number, so print entire line and throw an underline at the
    // right
    // position
    if (start_line_col.first == end_line_col.first) {
        out << get_line(range.file_data, start_line_col.first) << "\n";

        int current_col = 1;

        for (; current_col < start_line_col.second; current_col++) {
            out << " ";
        }

        out << "^";
        current_col++;

        for (; current_col < end_line_col.second; current_col++) {
            out << "~";
        }
    } else {
        int line_begin_index = range.start - 1;

        for (int i = line_begin_index; i <= range.end; ++i) {
            if (range.file_data[i] == '\n') {
                out << range.file_data[i];
            } else {
                out << range.file_data[i];
            }
        }

        // out << "\n";
        // out << range.file_data.substr(range.start, range.end - range.start);
    }


    return out;
}

bool PositionRange::operator==(const PositionRange& other) {
    return this->start == other.start
           && this->end == other.end;
}
