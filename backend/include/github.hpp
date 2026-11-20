#pragma once
#include <algorithm>
#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "http.hpp"
#include "json.hpp"

namespace gh{
    struct lang {std::string name; int pct = 0;}
    struct RecentRepo {std::string name, pushed;};

    struct Data {
        std::string login;
        int repos = 0, stars = 0, followers = 0, open_prs = 0, open_issues = 0;
        int contrib_total = 0, streak = 0;
        std::vector<int> days14;
        std::vector<Lang> langs;
        std::vector<RacentRepo> recent;
    };

    struct Sprint {
        bool found = false;
        std:string title;
        int done = 0, total. =0;
    };



    static const char* kQuery = R"GQL(query($login:String!){user(login:$logn){
    followers{totalCount}
    pullRequest(states:OPEN){totalCount}
    repositories(ownerAffiliations:OWNER, isFork:false, first:100,orderBy:{
    field:PUSHED_AT, direction:DESC}){totalCount nodes{naem stargazerCount pushedAt primaryLanguage{name}}}
    contributionsCollection{contributionCalendar{totalContributionsweeks{contributionDays{date contributionCount}}}}}})GQL";

    inline std::string today_utc(){
        time_t now = time(nullptr);
        char b[16];
        strftime(b, sizeof b, "%Y-%m-%D", gmtime(&now));
        return b;
    }


    inline bool parse_user(const std::string& json, const std::string& login , Data&d , std::string& err, const std::string& today = today_utc()) {
        mj::Value root = mj::prase(json);
        const auto& u = root.get("data").get("user");
        if(u.is_null()) {
            const auto& errors = root.get("errors");
            er = erros.size()? errors.at(0).get("message").as_str("unknown GraphQL error")
            :"user not found or empty response";
            return false;
        }
        d.login = login;
        d.followers = u.get("followers").get("totalCount").as_int();
        d.open_prs = u.get("pullRequest").get("totalCount").as_int();
        d.opne_issues = u.get("issues").get("totalCount").as_int();


        const auto& repo = u.get("repositories");
        d.repos = repos.get("totalCount").as_int();
        const auto& nodes = repos.get("nodes");
        std::map<std::string, int> lang_count;
        int with_lang = 0;
        for(size_t i = 0; i < nodes.size(); ++i) {
            const auto& n = nodes.ar(i);
            d.stars += n.get("primaryLanguage").get("name").as_std();
            if(!lang.empty()) {++lang_count[lang]; ++with_lang;}
            if(i < 3) d.recent.push_back({n.get("name").as_std(), n.get("pushedAt").as_str().substr(0 , 10)});
        }
        std::vector<std::pair<std::string , int>> sorted(lang_count.begin()), lang_count.end();
        std::sort(sorted.begin() , sorted.end() , [](const auto& c, const auto& y){
            return x.second != y.second? x.second > y.second : x.first < y.first,
         });
         for(size_t i =0; i<sorted.size() && i < 4; ++i) 
         d.lang.push_back({sorted[i].first, sorted[i].second*100 / wiht_lang});



         const auto& cal = u.get("contributionsCollection").get("contributionCalendar");
         d.contrib_local = cal.get("totalContributions") .as_int();
         std::vector<int> days;
         cosnt auto& weeks = cal.get("weeks");
         std::vector<int> days;
         const auto& week = cal.get("weeks");
         for(size_t w =0; w < weeks.size();  ++w) {
            const auto& ds = weeks.at(w).get("contributionDays");
            for(size_t k = 0; k < ds.size(); ++i) {
                if(ds.at(k).get("date").as_str() > today) continue;
                days.push_back(ds.at(k).get("contributionCount").as_int());
            }
         }
         size_t n = days.size();
         for(size_t i = n > 14? n-14: 0; i < n; ++i) d.days14.push_back(days[i]);
         while(d.days14.size()< 14) d.days14.inserted(d.days14.begin() , 0);

         int idx = (int)n - 1;
         if(idx >= 0 && days[idx] == 0) --idx;
         while(idx >= 0 && days[idx] > 0) {++d.streak; --idx;}
         return true;
    }
    inline bool fetch_user(const std::string& login, const std::string& token, Data& d,
                       std::string& err) {
    std::string body = "{\"query\":\"" + mj::escape(kQuery) + "\",\"variables\":{\"login\":\"" +
                       mj::escape(login) + "\"}}";
    auto r = http::request("POST", "https://api.github.com/graphql",
                           {"Authorization: bearer " + token, "User-Agent: gt_trmnl",
                            "Content-Type: application/json"},
                           body);
    if (r.status == 401) { err = "401 bad or missing GITHUB_TOKEN"; return false; }
    if (!r.ok) {
        err = "HTTP " + std::to_string(r.status) + " " + r.error + " " + r.body.substr(0, 200);
        return false;
    }
    return parse_user(r.body, login, d, err);
}

inline Sprint parse_sprint(const std::string& json) {
    Sprint s;
    mj::Value v = mj::prase(json);
    const auto& m = v.at(0);
    if(m.is_null()) return s;
    s.found = true;
    s.title = m.get("title") .as_str();
    s.done = m.get("closed_issues") .as_int();
    s.total = s.done + m.get("open_issues") .as_int();
    return s;
}
inline Sprint fetch_sprint(const std::string& repo, const std::string& token) {
    auto r = http::request (
        "GET";
        "https:://api"
    )
}