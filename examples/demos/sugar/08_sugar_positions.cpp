/*══════════════════════════════════════════════════════════════════════════════
 FILE: 08_sugar_positions.cpp - SUGAR POSITION INFORMATION (read-only)

 PURPOSE:
   Inspect open positions with the high-level helpers. Read-only: it lists and sums
   but does not close anything (closing helpers are listed at the end).

 📚 WHAT THIS DEMO COVERS:
   • getOpenPositions() / getPositionsBySymbol()
   • hasOpenPosition() / countOpenPositions()
   • getTotalProfit() / getProfitBySymbol()
   (closing: closePosition, closePositionPartial, closeAllPositions, closeAllBySymbol)

 🔄 API LEVELS:  HIGH-LEVEL (Sugar) — THIS FILE.

 🚀 HOW TO RUN:  ./build/demos 8
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

void demo_sugar_positions(MT5Sugar& sugar, const Config& cfg) {
    const std::string sym = cfg.testSymbol;

    printSub("Overview");
    std::cout << "  open positions : " << sugar.countOpenPositions() << "\n";
    std::cout << "  total profit   : " << sugar.getTotalProfit() << "\n";
    std::cout << "  has " << sym << " position: " << (sugar.hasOpenPosition(sym) ? "yes" : "no") << "\n";
    std::cout << "  profit on " << sym << "   : " << sugar.getProfitBySymbol(sym) << "\n";

    printSub("All open positions");
    auto positions = sugar.getOpenPositions();
    if (positions.empty()) {
        std::cout << "  (none — run demo [7] to open a short-lived test position)\n";
    } else {
        for (const auto& p : positions)
            std::cout << "  #" << p.ticket << "  " << p.symbol << "  " << (p.isBuy() ? "BUY " : "SELL")
                      << "  vol=" << p.volume << "  open=" << p.priceOpen
                      << "  cur=" << p.priceCurrent << "  P/L=" << p.profit << "\n";
    }

    printSub(std::string("Positions on ") + sym);
    auto bySym = sugar.getPositionsBySymbol(sym);
    std::cout << "  " << bySym.size() << " position(s) on " << sym << "\n";
}

} // namespace demos
