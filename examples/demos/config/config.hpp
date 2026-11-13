#pragma once
// ══════════════════════════════════════════════════════════════════════════════
// FILE: config.hpp - MT5 CONNECTION CONFIGURATION LOADER
// ══════════════════════════════════════════════════════════════════════════════
//
// 🎯 WHAT IS THIS?
//   Tiny, dependency-free loader for the connection settings every demo needs.
//
//  LOADING PRIORITY:
//   1. examples/demos/config/config.json  (if present next to the executable or in CWD)
//   2. Environment variables               (fallback / override)
//
// 📁 METHOD 1 — config.json (recommended):
//   {
//     "user": 5057947437,
//     "password": "YourPassword",
//     "grpc_server": "mt5.mrpc.pro:443",
//     "mt_cluster": "MetaQuotes-Demo",
//     "api_key": "",
//     "test_symbol": "EURUSD",
//     "test_volume": 0.01
//   }
//
// 🌍 METHOD 2 — environment variables:
//   MT5_LOGIN, MT5_PASSWORD, MT5_SERVER (cluster name), MT5_GRPC (host:port),
//   MT5_API_KEY, MT5_SYMBOL, MT5_VOLUME

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace demos {

struct Config {
    int64_t user{0};
    std::string password;
    std::string grpcServer{"mt5.mrpc.pro:443"};   // host:port of the gateway
    std::string mtCluster{"MetaQuotes-Demo"};     // broker server name for ConnectEx
    std::string apiKey;                           // "" -> TRIAL
    std::string testSymbol{"EURUSD"};
    double testVolume{0.01};
};

namespace detail {

// Minimal extractor for a flat JSON `"key": value` (string, number or bool). Good enough for
// the tiny config.json above; not a general JSON parser.
inline bool jsonField(const std::string& json, const std::string& key, std::string& out) {
    std::string needle = "\"" + key + "\"";
    auto k = json.find(needle);
    if (k == std::string::npos) return false;
    auto colon = json.find(':', k + needle.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t' || json[i] == '\n' || json[i] == '\r')) ++i;
    if (i >= json.size()) return false;
    if (json[i] == '"') {                                  // string value
        auto end = json.find('"', i + 1);
        if (end == std::string::npos) return false;
        out = json.substr(i + 1, end - i - 1);
    } else {                                               // number / bool
        auto end = json.find_first_of(",}\n\r", i);
        out = json.substr(i, (end == std::string::npos ? json.size() : end) - i);
        while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back();
    }
    return true;
}

inline std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

inline const char* env(const char* name) {
    const char* v = std::getenv(name);
    return (v && *v) ? v : nullptr;
}

} // namespace detail

// Load the config: start from config.json (searched in a few common locations), then let any
// present environment variable override. Returns a Config with sensible defaults either way.
inline Config loadConfig() {
    Config c;

    // 1) config.json — try a few locations relative to where the demo is run from.
    const char* candidates[] = {
        "config/config.json",
        "examples/demos/config/config.json",
        "../config/config.json",
        "config.json",
    };
    std::string json;
    for (const char* p : candidates) { json = detail::readFile(p); if (!json.empty()) break; }
    if (!json.empty()) {
        std::string v;
        if (detail::jsonField(json, "user", v))        c.user = std::strtoll(v.c_str(), nullptr, 10);
        if (detail::jsonField(json, "password", v))    c.password = v;
        if (detail::jsonField(json, "grpc_server", v)) c.grpcServer = v;
        if (detail::jsonField(json, "mt_cluster", v))  c.mtCluster = v;
        if (detail::jsonField(json, "api_key", v))     c.apiKey = v;
        if (detail::jsonField(json, "test_symbol", v)) c.testSymbol = v;
        if (detail::jsonField(json, "test_volume", v)) c.testVolume = std::strtod(v.c_str(), nullptr);
    }

    // 2) environment overrides.
    if (const char* v = detail::env("MT5_LOGIN"))    c.user = std::strtoll(v, nullptr, 10);
    if (const char* v = detail::env("MT5_PASSWORD")) c.password = v;
    if (const char* v = detail::env("MT5_SERVER"))   c.mtCluster = v;
    if (const char* v = detail::env("MT5_GRPC"))     c.grpcServer = v;
    if (const char* v = detail::env("MT5_API_KEY"))  c.apiKey = v;
    if (const char* v = detail::env("MT5_SYMBOL"))   c.testSymbol = v;
    if (const char* v = detail::env("MT5_VOLUME"))   c.testVolume = std::strtod(v, nullptr);

    return c;
}

} // namespace demos
