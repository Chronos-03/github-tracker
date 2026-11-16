#pragma once
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace kicad {

struct BoardStats {
    int copper_layers = 0;
    int vias = 0;
    int trace_segments = 0;
};

struct BomStats {
    int total_parts = 0;
    int missing_lcsc = 0;
};

inline std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

inline BoardStats analyze_pcb(const std::string& text) {
    BoardStats s;
    int depth = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '"') {
            for (++i; i < text.size() && text[i] != '"'; ++i)
                if (text[i] == '\\') ++i;
            continue;
        }
        if (c == '(') {
            ++depth;
            if (depth == 2) {
                size_t j = i + 1, k = j;
                while (k < text.size() && !std::isspace((unsigned char)text[k]) &&
                       text[k] != '(' && text[k] != ')')
                    ++k;
                std::string tag = text.substr(j, k - j);
                if (tag == "via") ++s.vias;
                else if (tag == "segment") ++s.trace_segments;
                else if (tag == "layers") {
                    int d = 1;
                    size_t p = k;
                    while (p < text.size() && d > 0) {
                        if (text[p] == '(') ++d;
                        else if (text[p] == ')') --d;
                        else if (text[p] == '"') {
                            size_t e = text.find('"', p + 1);
                            if (e == std::string::npos) break;
                            std::string name = text.substr(p + 1, e - p - 1);
                            if (name.size() > 3 &&
                                name.compare(name.size() - 3, 3, ".Cu") == 0)
                                ++s.copper_layers;
                            p = e;
                        }
                        ++p;
                    }
                }
            }
        } else if (c == ')') {
            --depth;
        }
    }
    return s;
}

inline std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool in_q = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_q) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
            else if (c == '"') in_q = false;
            else cur += c;
        } else if (c == '"') in_q = true;
        else if (c == ',') { out.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    out.push_back(cur);
    return out;
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

inline BomStats analyze_bom(const std::string& csv) {
    BomStats b;
    std::istringstream in(csv);
    std::string line;
    int lcsc_col = -1;
    if (!std::getline(in, line)) return b;
    auto header = split_csv_line(line);
    for (size_t i = 0; i < header.size(); ++i)
        if (lower(header[i]).find("lcsc") != std::string::npos) { lcsc_col = (int)i; break; }
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto row = split_csv_line(line);
        ++b.total_parts;
        if (lcsc_col < 0 || lcsc_col >= (int)row.size() || row[lcsc_col].empty())
            ++b.missing_lcsc;
    }
    return b;
}

}