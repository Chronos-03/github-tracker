#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "kicad_analyzer.hpp"

inline std::string make_sprint_svg(double hrs_done, double hrs_goal, const std::vector<int>& daily_commits) {
    const int w = 350;
    const int h = 42;

    double ratio = (hrs_goal > 0.0) ? (hrs_done / hrs_goal) : 0.0;
    if (ratio > 1.0) ratio = 1.0;
    if (ratio < 0.0) ratio = 0.0;

    int fill_w = static_cast<int>((w - 8) * ratio);
    std::ostringstream ss;
    ss << "<svg width=\"" << w << "\" height=\"" << h << "\" viewBox=\"0 0 " << w << " " << h
       << "\" xmlns=\"http://www.w3.org/2000/svg\" shape-rendering=\"crispEdges\">";

    ss << "<rect x=\"0\" y=\"0\" width=\"" << w << "\" height=\"20\" fill=\"#ffffff\" stroke=\"#000000\" stroke-width=\"2\"/>";
    if (fill_w > 0) {
        ss << "<rect x=\"4\" y=\"4\" width=\"" << fill_w << "\" height=\"12\" fill=\"#000000\"/>";
    }

    int x = 0;
    for (size_t i = 0; i < daily_commits.size() && i < 14; i++) {
        const char* color = (daily_commits[i] > 0) ? "#000000" : "#ffffff";
        ss << "<rect x=\"" << x << "\" y=\"26\" width=\"21\" height=\"12\" fill=\"" << color
           << "\" stroke=\"#000000\" stroke-width=\"2\"/>";
        x += 25;
    }

    ss << "</svg>";
    return ss.str();
}

inline bool write_trmnlp_yml(const std::string& out_path, const std::string& repo, const pcb_stats& st, double hrs, const std::string& svg) {
    std::ofstream out(out_path);
    if (!out.is_open()) return false;

    out << "watch:\n  - src\n  - .trmnlp.yml\n\n";
    out << "variables:\n";
    out << "  repo_name: \"" << repo << "\"\n";
    out << "  branch: \"main\"\n";
    out << "  drc_status: \"" << (st.missing_lcsc == 0 ? "READY" : "WARN") << "\"\n";
    out << "  board_dims: \"" << std::fixed << std::setprecision(1) << st.width_mm << " x " << st.height_mm << " mm\"\n";
    out << "  copper_layers: " << st.layers << "\n";
    out << "  track_segments: " << st.segments << "\n";
    out << "  total_vias: " << st.vias << "\n";
    out << "  total_nets: " << st.nets << "\n";
    out << "  smd_pads: " << st.smd_pads << "\n";
    out << "  tht_pads: " << st.tht_pads << "\n";
    out << "  unique_parts: " << st.unique_parts << "\n";
    out << "  total_parts: " << st.total_parts << "\n";
    out << "  missing_lcsc: " << st.missing_lcsc << "\n";
    out << "  bom_cost_usd: \"" << std::fixed << std::setprecision(2) << st.bom_cost << "\"\n";
    out << "  hours_logged: \"" << std::fixed << std::setprecision(1) << hrs << " / 18.0 hrs\"\n";
    out << "  progress_svg: |\n    " << svg << "\n";
    out << "  missing_list:\n";
    for (const auto& item : st.unassigned) {
        out << "    - ref: \"" << item.ref << "\"\n";
        out << "      val: \"" << item.val << "\"\n";
        out << "      fp: \"" << item.footprint << "\"\n";
    }
    return true;
}