#include <curl/curl.h>

#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "eink_svg.hpp"
#include "kicad_analyzer.hpp"

struct Args {
    std::string pcb, bom, repo = ".", out, webhook, name = "my-board";
    int done = 0, total = 0;
};

struct Telemetry {
    std::string name, progress_svg, commit_svg, updated_at;
    int layers = 0, vias = 0, traces = 0, parts = 0, missing_lcsc = 0;
    int sprint_done = 0, sprint_total = 0, sprint_pct = 0;
};

static std::vector<int> commits_last_14_days(const std::string& repo) {
    std::map<std::string, int> per_day;
    std::string cmd = "git -C \"" + repo +
                      "\" log --since=14.days.ago --format=%ad --date=short 2>/dev/null";
    if (FILE* p = popen(cmd.c_str(), "r")) {
        char buf[64];
        while (fgets(buf, sizeof buf, p)) {
            std::string d(buf);
            while (!d.empty() && (d.back() == '\n' || d.back() == '\r')) d.pop_back();
            ++per_day[d];
        }
        pclose(p);
    }
    std::vector<int> out;
    time_t now = time(nullptr);
    for (int i = 13; i >= 0; --i) {
        time_t t = now - (time_t)i * 86400;
        char b[16];
        strftime(b, sizeof b, "%Y-%m-%d", localtime(&t));
        out.push_back(per_day[b]);
    }
    return out;
}

static std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

static std::string to_json(const Telemetry& t) {
    std::string j = "{\"merge_variables\":{";
    j += "\"name\":\"" + json_escape(t.name) + "\",";
    j += "\"layers\":" + std::to_string(t.layers) + ",";
    j += "\"vias\":" + std::to_string(t.vias) + ",";
    j += "\"traces\":" + std::to_string(t.traces) + ",";
    j += "\"parts\":" + std::to_string(t.parts) + ",";
    j += "\"missing_lcsc\":" + std::to_string(t.missing_lcsc) + ",";
    j += "\"sprint_done\":" + std::to_string(t.sprint_done) + ",";
    j += "\"sprint_total\":" + std::to_string(t.sprint_total) + ",";
    j += "\"sprint_pct\":" + std::to_string(t.sprint_pct) + ",";
    j += "\"progress_svg\":\"" + json_escape(t.progress_svg) + "\",";
    j += "\"commit_svg\":\"" + json_escape(t.commit_svg) + "\",";
    j += "\"updated_at\":\"" + json_escape(t.updated_at) + "\"}}";
    return j;
}

// Writes plugin/.trmnlp.yml so `trmnlp serve` previews real data.
static bool write_trmnlp_yml(const std::string& path, const Telemetry& t) {
    std::ofstream f(path);
    if (!f) return false;
    f << "---\n"
      << "watch_paths:\n  - src\n"
      << "custom_fields: {}\n"
      << "variables:\n"
      << "  name: '" << t.name << "'\n"
      << "  layers: " << t.layers << "\n"
      << "  vias: " << t.vias << "\n"
      << "  traces: " << t.traces << "\n"
      << "  parts: " << t.parts << "\n"
      << "  missing_lcsc: " << t.missing_lcsc << "\n"
      << "  sprint_done: " << t.sprint_done << "\n"
      << "  sprint_total: " << t.sprint_total << "\n"
      << "  sprint_pct: " << t.sprint_pct << "\n"
      << "  progress_svg: '" << t.progress_svg << "'\n"
      << "  commit_svg: '" << t.commit_svg << "'\n"
      << "  updated_at: '" << t.updated_at << "'\n";
    return true;
}

static bool push_webhook(const std::string& url, const std::string& json) {
    CURL* c = curl_easy_init();
    if (!c) return false;
    curl_slist* h = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, json.c_str());
    CURLcode rc = curl_easy_perform(c);
    long code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    std::cerr << "webhook: curl=" << rc << " http=" << code << "\n";
    return rc == CURLE_OK && code >= 200 && code < 300;
}

static Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--pcb") a.pcb = next();
        else if (k == "--bom") a.bom = next();
        else if (k == "--repo") a.repo = next();
        else if (k == "--out") a.out = next();
        else if (k == "--webhook") a.webhook = next();
        else if (k == "--name") a.name = next();
        else if (k == "--done") a.done = std::atoi(next().c_str());
        else if (k == "--total") a.total = std::atoi(next().c_str());
    }
    return a;
}

int main(int argc, char** argv) {
    Args a = parse(argc, argv);
    if (a.pcb.empty() || a.bom.empty()) {
        std::cerr << "usage: gt_trmnl --pcb board.kicad_pcb --bom bom.csv\n"
                     "  [--repo path] [--name label] [--done N --total N]\n"
                     "  [--out plugin/.trmnlp.yml] [--webhook URL]\n";
        return 1;
    }
    std::string pcb = kicad::read_file(a.pcb), csv = kicad::read_file(a.bom);
    if (pcb.empty() || csv.empty()) {
        std::cerr << "could not read input files\n";
        return 1;
    }

    auto board = kicad::analyze_pcb(pcb);
    auto bom = kicad::analyze_bom(csv);

    Telemetry t;
    t.name = a.name;
    t.layers = board.copper_layers;
    t.vias = board.vias;
    t.traces = board.trace_segments;
    t.parts = bom.total_parts;
    t.missing_lcsc = bom.missing_lcsc;
    t.sprint_done = a.done;
    t.sprint_total = a.total;
    t.sprint_pct = a.total > 0 ? a.done * 100 / a.total : 0;
    t.progress_svg = eink::progress_bar(a.done, a.total);
    t.commit_svg = eink::commit_grid(commits_last_14_days(a.repo));
    char ts[32];
    time_t now = time(nullptr);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M", localtime(&now));
    t.updated_at = ts;

    std::cout << "layers " << t.layers << " | vias " << t.vias << " | traces " << t.traces
              << " | parts " << t.parts << " | missing LCSC " << t.missing_lcsc << "\n";

    if (!a.out.empty()) {
        if (!write_trmnlp_yml(a.out, t)) { std::cerr << "cannot write " << a.out << "\n"; return 1; }
        std::cout << "wrote " << a.out << "\n";
    }
    if (!a.webhook.empty() && !push_webhook(a.webhook, to_json(t))) return 1;
    return 0;
}