#pragma once
#include <sstream>
#include <string>
#include <vector>
namespace eink {
inline std::string progress_bar(int done, int total, int w = 360, int h = 24) {
    double frac = total > 0 ? (double)done / total : 0.0;
    if (frac > 1.0) frac = 1.0;
    int fill = (int)((w - 4) * frac);
    std::ostringstream s;
    s << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << w << "\" height=\"" << h
      << "\" viewBox=\"0 0 " << w << " " << h << "\" shape-rendering=\"crispEdges\">"
      << "<rect x=\"1\" y=\"1\" width=\"" << w - 2 << "\" height=\"" << h - 2
      << "\" fill=\"white\" stroke=\"black\" stroke-width=\"2\"/>"
      << "<rect x=\"2\" y=\"2\" width=\"" << fill << "\" height=\"" << h - 4
      << "\" fill=\"black\"/></svg>";
    return s.str();
}
inline std::string commit_grid(const std::vector<int>& counts) {
    const int cell = 20, gap = 4, cols = 7;
    int rows = ((int)counts.size() + cols - 1) / cols;
    int w = cols * (cell + gap) - gap, h = rows * (cell + gap) - gap;
    std::ostringstream s;
    s << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << w << "\" height=\"" << h
      << "\" viewBox=\"0 0 " << w << " " << h << "\" shape-rendering=\"crispEdges\">"
      << "<g fill=\"black\" stroke=\"black\" stroke-width=\"2\">";
    for (size_t i = 0; i < counts.size(); ++i) {
        int x = (int)(i % cols) * (cell + gap), y = (int)(i / cols) * (cell + gap);
        int n = counts[i];
        if (n >= 3) {
            s << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << cell
              << "\" height=\"" << cell << "\"/>";
        } else {
            s << "<rect x=\"" << x + 1 << "\" y=\"" << y + 1 << "\" width=\"" << cell - 2
              << "\" height=\"" << cell - 2 << "\" fill=\"white\"/>";
            if (n > 0)
                s << "<rect x=\"" << x + 6 << "\" y=\"" << y + 6
                  << "\" width=\"8\" height=\"8\" stroke=\"none\"/>";
        }
    }
    s << "</g></svg>";
    return s.str();
}
}