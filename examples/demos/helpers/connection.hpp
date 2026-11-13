#pragma once
// ══════════════════════════════════════════════════════════════════════════════
// FILE: connection.hpp - SHARED DEMO HELPERS (connection + pretty printing)
// ══════════════════════════════════════════════════════════════════════════════
//
// Centralised connection for every demo. connect() builds one MT5Sugar (which owns an
// MT5Service and an MT5Account) and runs ConnectEx, so a single object gives you all three
// layers:  sugar.account()  (low-level)  ·  sugar.service()  (mid-level)  ·  sugar (high-level).

#include <iostream>
#include <memory>
#include <string>

#include "metarpc/mt5_sugar.hpp"
#include "metarpc/mt5_account.hpp"   // complete type for sugar.account() (pulls protobuf)
#include "config/config.hpp"

namespace demos {

// A framed section header, e.g.  ══ CONNECTION ══
inline void printSection(const std::string& title) {
    std::cout << "\n";
    std::cout << "══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "══════════════════════════════════════════════════════════════════════════════\n";
}

// A lighter sub-heading.
inline void printSub(const std::string& title) {
    std::cout << "\n── " << title << " " << std::string(title.size() < 60 ? 60 - title.size() : 0, '-') << "\n";
}

// Build an MT5Sugar from the config and connect. Throws metarpc::ApiError / TransportError on
// failure (the demo's main() catches and reports it).
inline std::unique_ptr<metarpc::MT5Sugar> connect(const Config& cfg) {
    printSection("CONNECTION");
    std::cout << "  User:        " << cfg.user << "\n";
    std::cout << "  gRPC Server: " << cfg.grpcServer << "\n";
    std::cout << "  MT Cluster:  " << cfg.mtCluster << "\n";
    std::cout << "  Symbol:      " << cfg.testSymbol << "\n";
    std::cout << "  API key:     " << (cfg.apiKey.empty() ? "(TRIAL)" : "(set)") << "\n";

    // grpcServer is "host:port" — split it for the MT5Account constructor.
    std::string host = cfg.grpcServer; int port = 443;
    auto colon = cfg.grpcServer.rfind(':');
    if (colon != std::string::npos) {
        host = cfg.grpcServer.substr(0, colon);
        port = std::atoi(cfg.grpcServer.c_str() + colon + 1);
    }
    auto sugar = std::make_unique<metarpc::MT5Sugar>(host, port, cfg.apiKey);
    sugar->connect(cfg.user, cfg.password, cfg.mtCluster);
    std::cout << "  ✅ Connected. Terminal id: " << sugar->account().terminalGuid() << "\n";
    return sugar;
}

} // namespace demos
