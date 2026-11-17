#pragma once
#include <curl/curl.h>
#include <string>
#include <vector>
namespace http {

struct Response {
    long status = 0;
    std::string body, error;
    bool ok = false;
};

inline size_t write_cb(char* p, size_t sz, size_t n, void* ud) {
    static_cast<std::string*>(ud)->append(p, sz * n);
    return sz * n;
}

inline Response request(const std::string& method, const std::string& url,
                        const std::vector<std::string>& headers,
                        const std::string& body = "") {
    Response r;
    CURL* c = curl_easy_init();
    if (!c) { r.error = "curl init failed"; return r; }
    curl_slist* h = nullptr;
    for (const auto& s : headers) h = curl_slist_append(h, s.c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    if (method == "POST") {
        curl_easy_setopt(c, CURLOPT_POST, 1L);
        curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, (long)body.size());
    }
    CURLcode rc = curl_easy_perform(c);
    if (rc != CURLE_OK) r.error = curl_easy_strerror(rc);
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &r.status);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    r.ok = rc == CURLE_OK && r.status >= 200 && r.status < 300;
    return r;
}

} 