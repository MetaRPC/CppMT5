/*══════════════════════════════════════════════════════════════════════════════
 FILE: 07_sugar_trading.cpp - SUGAR SIMPLE TRADING  (⚠ PLACES REAL DEMO ORDERS)

 PURPOSE:
   The one-liner trading helpers. This demo opens a tiny market position, shows it,
   then closes it again — a full round-trip on the DEMO account.

 ⚠ SAFETY:
   This runs live trades on your configured (demo) account. Volume = config test_volume
   (0.01 by default). It closes what it opens before returning.

 📚 WHAT THIS DEMO COVERS:
   • buyMarket()            -> open a market BUY, returns the ticket
   • getPositionByTicket()  -> read it back
   • modifyPositionSLTP()   -> attach SL/TP (pips → price via calculateSLTP)
   • closePosition()        -> close the whole position
   (also available: sellMarket, buy/sellLimit/Stop, *WithSLTP, *WithPips)

 🔄 API LEVELS:  HIGH-LEVEL (Sugar) — THIS FILE.

 🚀 HOW TO RUN:  ./build/demos 7
══════════════════════════════════════════════════════════════════════════════*/

#include <chrono>
#include <iostream>
#include <thread>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

void demo_sugar_trading(MT5Sugar& sugar, const Config& cfg) {
    const std::string sym = cfg.testSymbol;
    const double vol = cfg.testVolume;

    printSub(std::string("Open a market BUY ") + std::to_string(vol) + " " + sym);
    uint64_t ticket = sugar.buyMarket(sym, vol);
    if (ticket == 0) { std::cout << "  order was rejected (market closed / insufficient margin?)\n"; return; }
    std::cout << "  opened position #" << ticket << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    printSub("Read it back");
    bool found = false;
    Position p = sugar.getPositionByTicket((int64_t)ticket, found);
    if (found)
        std::cout << "  #" << p.ticket << "  " << p.symbol << "  " << (p.isBuy() ? "BUY" : "SELL")
                  << "  vol=" << p.volume << "  open=" << p.priceOpen << "  profit=" << p.profit << "\n";

    printSub("Attach SL/TP (30 / 60 pips) then close");
    double entry = p.priceOpen, sl = 0, tp = 0;
    sugar.calculateSLTP(sym, "buy", entry, 30, 60, sl, tp);
    std::cout << "  modify SL=" << sl << " TP=" << tp << "  -> "
              << (sugar.modifyPositionSLTP((int64_t)ticket, sl, tp) ? "ok" : "failed") << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "  close #" << ticket << "  -> "
              << (sugar.closePosition((int64_t)ticket) ? "closed" : "FAILED") << "\n";
}

} // namespace demos
