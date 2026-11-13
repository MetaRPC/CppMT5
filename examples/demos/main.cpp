/*══════════════════════════════════════════════════════════════════════════════
 FILE: main.cpp - CppMT5 DEMO LAUNCHER

 Interactive menu (or direct launch) for the CppMT5 examples. Connects once, then runs
 the chosen demo against the live demo account configured in config/config.json.

 🚀 HOW TO RUN:
   cmake --build build --target demos
   ./build/demos            → interactive menu
   ./build/demos 6          → run demo [6] directly
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>
#include <memory>
#include <string>

#include "metarpc/mt5_sugar.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"
#include "demos.hpp"

namespace {

void printMenu() {
    std::cout << "\n";
    std::cout << "══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  CppMT5 DEMOS\n";
    std::cout << "══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  Low-level (MT5Account, raw protobuf):\n";
    std::cout << "    [1]  General operations   - account & symbols (raw)\n";
    std::cout << "    [2]  Trade calc & check   - margin/profit/orderCheck (read-only)\n";
    std::cout << "    [3]  Streaming            - raw tick stream (~5s)\n";
    std::cout << "  Mid-level (MT5Service, friendly structs):\n";
    std::cout << "    [4]  Service demo         - AccountInfo/Tick/SymbolInfo/bars\n";
    std::cout << "    [5]  Service streaming    - friendly Tick + TradeEvent streams\n";
    std::cout << "  High-level (MT5Sugar):\n";
    std::cout << "    [6]  Sugar basics         - connection, balance, prices\n";
    std::cout << "    [7]  Sugar trading        - ⚠ opens & closes a tiny demo position\n";
    std::cout << "    [8]  Sugar positions      - list open positions (read-only)\n";
    std::cout << "    [9]  Sugar history        - deals & daily stats (read-only)\n";
    std::cout << "    [10] Sugar advanced       - risk/sizing helpers (read-only)\n";
    std::cout << "  Orchestrators & presets:\n";
    std::cout << "    [11] Trailing stop        - ⚠ opens, trails SL, closes\n";
    std::cout << "    [13] Grid trader          - ⚠ places & removes a pending grid\n";
    std::cout << "    [14] Risk manager         - account monitor / kill-switch\n";
    std::cout << "    [16] Adaptive preset      - analyse market → pick an orchestrator\n";
    std::cout << "\n  [0]  exit\n";
    std::cout << "  Select: ";
}

// Dispatch a demo by id. Returns false for an unknown id.
bool run(const std::string& which, metarpc::MT5Sugar& sugar, const demos::Config& cfg) {
    if (which == "1")  { demos::demo_general_operations(sugar, cfg); return true; }
    if (which == "2")  { demos::demo_trading_operations(sugar, cfg); return true; }
    if (which == "3")  { demos::demo_streaming_methods(sugar, cfg);  return true; }
    if (which == "4")  { demos::demo_service_demo(sugar, cfg);       return true; }
    if (which == "5")  { demos::demo_service_streaming(sugar, cfg);  return true; }
    if (which == "6")  { demos::demo_sugar_basics(sugar, cfg);       return true; }
    if (which == "7")  { demos::demo_sugar_trading(sugar, cfg);      return true; }
    if (which == "8")  { demos::demo_sugar_positions(sugar, cfg);    return true; }
    if (which == "9")  { demos::demo_sugar_history(sugar, cfg);      return true; }
    if (which == "10") { demos::demo_sugar_advanced(sugar, cfg);     return true; }
    if (which == "11") { demos::demo_trailing_stop(sugar, cfg);      return true; }
    if (which == "13") { demos::demo_grid_trader(sugar, cfg);        return true; }
    if (which == "14") { demos::demo_risk_manager(sugar, cfg);       return true; }
    if (which == "16") { demos::demo_adaptive_preset(sugar, cfg);    return true; }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    demos::Config cfg = demos::loadConfig();
    if (cfg.user == 0 || cfg.password.empty()) {
        std::cerr << "No credentials. Fill examples/demos/config/config.json or set "
                     "MT5_LOGIN / MT5_PASSWORD / MT5_SERVER.\n";
        return 2;
    }

    try {
        auto sugar = demos::connect(cfg);

        if (argc > 1) {                          // direct launch:  demos <id>
            if (!run(argv[1], *sugar, cfg)) {
                std::cerr << "Unknown demo: " << argv[1] << "\n";
                return 1;
            }
            return 0;
        }

        for (;;) {                               // interactive menu
            printMenu();
            std::string choice;
            if (!std::getline(std::cin, choice)) break;
            if (choice == "0" || choice == "exit" || choice == "quit") break;
            if (!run(choice, *sugar, cfg))
                std::cout << "  Unknown choice: " << choice << "\n";
            std::cout << "\nPress Enter to return to the menu...";
            std::string dummy; std::getline(std::cin, dummy);
        }
    } catch (const metarpc::ApiError& e) {
        std::cerr << "\n❌ ApiError: " << e.what() << "\n";
        return 1;
    } catch (const metarpc::TransportError& e) {
        std::cerr << "\n❌ TransportError: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ " << e.what() << "\n";
        return 1;
    }
    return 0;
}
