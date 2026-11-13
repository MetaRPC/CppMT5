/*══════════════════════════════════════════════════════════════════════════════
 FILE: 06_sugar_basics.cpp - HIGH-LEVEL SUGAR API BASICS DEMO

 PURPOSE:
   Demonstrates MT5Sugar basics: connection health, account balance, and price
   queries. This is the SIMPLEST way to talk to MT5 — perfect for quick scripts.

 🎯 WHO SHOULD USE THIS:
   • Beginners learning the MT5 API
   • Quick prototyping and testing
   • Simple monitoring scripts

 📚 WHAT THIS DEMO COVERS (3 categories):

   1. CONNECTION METHODS (3)
      • isConnected() - connection status
      • ping()        - round-trip health check
      • account()     - reach the low-level layer for the terminal id

   2. BALANCE / MARGIN METHODS (6)
      • getBalance()      - account balance
      • getEquity()       - current equity
      • getMargin()       - used margin
      • getFreeMargin()   - available margin
      • getMarginLevel()  - margin level %
      • getProfit()       - floating P&L

   3. PRICES & QUOTES (4)
      • getBid()       - current BID
      • getAsk()       - current ASK
      • getSpread()    - ask - bid (price units)
      • getPriceInfo() - bid/ask/spread/time in one call

 🔄 API LEVELS:
   HIGH-LEVEL (Sugar)   - THIS FILE: one-liners, smart defaults
   MID-LEVEL (Service)  - friendly structs, more control      → sugar.service()
   LOW-LEVEL (Account)  - raw protobuf, maximum flexibility    → sugar.account()

 🚀 HOW TO RUN:
   cmake --build build --target demos      (once)
   ./build/demos 6                         (or pick [6] from the menu)
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"            // all three layers (MT5Account complete)
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;             // friendly types: PriceInfo, Tick, Position, …

void demo_sugar_basics(MT5Sugar& sugar, const Config& cfg) {
    const std::string sym = cfg.testSymbol;

    // ── 1. CONNECTION ────────────────────────────────────────────────────────
    printSub("Connection");
    std::cout << "  isConnected : " << (sugar.isConnected() ? "yes" : "no") << "\n";
    std::cout << "  ping        : " << (sugar.ping() ? "ok" : "FAILED") << "\n";
    std::cout << "  terminal id : " << sugar.account().terminalGuid() << "\n";

    // ── 2. BALANCE / MARGIN ──────────────────────────────────────────────────
    printSub("Balance / margin");
    std::cout << "  balance     : " << sugar.getBalance() << "\n";
    std::cout << "  equity      : " << sugar.getEquity() << "\n";
    std::cout << "  margin      : " << sugar.getMargin() << "\n";
    std::cout << "  free margin : " << sugar.getFreeMargin() << "\n";
    std::cout << "  margin level: " << sugar.getMarginLevel() << " %\n";
    std::cout << "  profit      : " << sugar.getProfit() << "\n";

    // ── 3. PRICES & QUOTES ───────────────────────────────────────────────────
    printSub(std::string("Prices for ") + sym);
    std::cout << "  bid    : " << sugar.getBid(sym) << "\n";
    std::cout << "  ask    : " << sugar.getAsk(sym) << "\n";
    std::cout << "  spread : " << sugar.getSpread(sym) << "\n";
    PriceInfo pi = sugar.getPriceInfo(sym);
    std::cout << "  priceInfo: bid=" << pi.bid << " ask=" << pi.ask
              << " spread=" << pi.spread << " time=" << pi.time << "\n";
}

} // namespace demos
