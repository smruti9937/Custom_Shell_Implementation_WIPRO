#include "parser.h"
#include <sstream>

// A simple tokenizer that keeps quoted strings together.
std::vector<std::string> tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::string cur;
    bool in_double = false;
    bool in_single = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' && !in_single) {
            in_double = !in_double;
            if (!in_double) {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            }
            continue;
        }
        if (c == '\'' && !in_double) {
            in_single = !in_single;
            if (!in_single) {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            }
            continue;
        }
        if (!in_double && !in_single && isspace((unsigned char)c)) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}
