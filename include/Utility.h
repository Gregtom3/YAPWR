#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace Utility {

/// Split a string by delim into a vector of substrings
inline std::vector<std::string> split(const std::string& line, char delim) {
    std::vector<std::string> out;
    std::istringstream ss(line);
    std::string item;
    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

/// Trim whitespace (spaces, tabs, newlines) from both ends
inline std::string trim(const std::string& s) {
    const char* ws = " \t\n\r";
    auto start = s.find_first_not_of(ws);
    if (start == std::string::npos)
        return {};
    auto end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

} // namespace Utility
