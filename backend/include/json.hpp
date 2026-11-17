#pragma once
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace mj {

inline std::string escape(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += (char)c;
        }
    }
    return o;
}

struct Value {
    enum Type { Null, Bool, Num, Str, Arr, Obj };
    Type type = Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Value> a;
    std::vector<std::pair<std::string, Value>> o;

    const Value& get(const std::string& key) const {
        static const Value none;
        for (const auto& kv : o)
            if (kv.first == key) return kv.second;
        return none;
    }
    const Value& at(size_t i) const {
        static const Value none;
        return i < a.size() ? a[i] : none;
    }
    bool is_null() const { return type == Null; }
    size_t size() const { return a.size(); }
    int as_int(int def = 0) const { return type == Num ? (int)n : def; }
    std::string as_str(const std::string& def = "") const { return type == Str ? s : def; }
};

class Parser {
public:
    explicit Parser(const std::string& text) : t_(text) {}
    Value parse() { return value(); }

private:
    const std::string& t_;
    size_t i_ = 0;

    char peek() const { return i_ < t_.size() ? t_[i_] : '\0'; }
    void ws() { while (i_ < t_.size() && std::isspace((unsigned char)t_[i_])) ++i_; }

    Value value() {
        ws();
        Value v;
        char c = peek();
        if (c == '{') {
            ++i_; v.type = Value::Obj; ws();
            if (peek() == '}') { ++i_; return v; }
            while (i_ < t_.size()) {
                ws();
                std::string k = str();
                ws();
                if (peek() == ':') ++i_;
                Value x = value();
                v.o.emplace_back(std::move(k), std::move(x));
                ws();
                if (peek() == ',') { ++i_; continue; }
                if (peek() == '}') ++i_;
                break;
            }
        } else if (c == '[') {
            ++i_; v.type = Value::Arr; ws();
            if (peek() == ']') { ++i_; return v; }
            while (i_ < t_.size()) {
                v.a.push_back(value());
                ws();
                if (peek() == ',') { ++i_; continue; }
                if (peek() == ']') ++i_;
                break;
            }
        } else if (c == '"') {
            v.type = Value::Str;
            v.s = str();
        } else if (c == 't' && t_.compare(i_, 4, "true") == 0) {
            v.type = Value::Bool; v.b = true; i_ += 4;
        } else if (c == 'f' && t_.compare(i_, 5, "false") == 0) {
            v.type = Value::Bool; i_ += 5;
        } else if (c == 'n' && t_.compare(i_, 4, "null") == 0) {
            i_ += 4;
        } else if (c == '-' || std::isdigit((unsigned char)c)) {
            char* end = nullptr;
            v.n = std::strtod(t_.c_str() + i_, &end);
            v.type = Value::Num;
            i_ = (size_t)(end - t_.c_str());
        } else {
            ++i_; 
        }
        return v;
    }

    std::string str() {
        std::string out;
        if (peek() != '"') { ++i_; return out; }
        ++i_;
        while (i_ < t_.size() && t_[i_] != '"') {
            char c = t_[i_++];
            if (c != '\\') { out += c; continue; }
            char e = peek();
            ++i_;
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    std::string hex = i_ <= t_.size() ? t_.substr(i_, 4) : "";
                    unsigned cp = (unsigned)std::strtoul(hex.c_str(), nullptr, 16);
                    i_ += 4;
                    if (cp < 0x80) out += (char)cp;
                    else if (cp < 0x800) {
                        out += (char)(0xC0 | (cp >> 6));
                        out += (char)(0x80 | (cp & 0x3F));
                    } else {
                        out += (char)(0xE0 | (cp >> 12));
                        out += (char)(0x80 | ((cp >> 6) & 0x3F));
                        out += (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: out += e; 
            }
        }
        ++i_; 
        return out;
    }
};

inline Value parse(const std::string& text) { return Parser(text).parse(); }
class Writer {
public:
    void num(const std::string& k, long v) { key(k); s_ += std::to_string(v); }
    void str(const std::string& k, const std::string& v) { key(k); s_ += "\"" + escape(v) + "\""; }
    void boolean(const std::string& k, bool v) { key(k); s_ += v ? "true" : "false"; }
    void raw(const std::string& k, const std::string& v) { key(k); s_ += v; }
    std::string done() const { return s_ + "}"; }

private:
    std::string s_ = "{";
    bool first_ = true;
    void key(const std::string& k) {
        if (!first_) s_ += ',';
        first_ = false;
        s_ += "\"" + escape(k) + "\":";
    }
};

} 