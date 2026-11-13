/*══════════════════════════════════════════════════════════════════════════════
 FILE: 10_sugar_advanced.cpp - SUGAR RISK & SYMBOL HELPERS (read-only)

 PURPOSE:
   Position sizing and pre-trade checks — all computed from live data without
   placing anything. The maths (pip value, margin, SL/TP prices) is done for you.

 📚 WHAT THIS DEMO COVERS (2 categories):

   1. SYMBOL HELPERS
      • isSymbolAvailable / getSymbolDigits / getMinStopLevel
      • getAllSymbols()  -> count

   2. RISK MANAGEMENT
      • calculateRequiredMargin()  -> margin for N lots
      • getMaxLotSize()            -> biggest lot free margin allows
      • calculatePositionSize()    -> lots for a % risk over a pip stop
      • canOpenPosition()          -> margin check + reason
      • calculateSLTP()            -> absolute SL/TP from pip distances

 🔄 API LEVELS:  HIGH-LEVEL (Sugar) — THIS FILE.

 🚀 HOW TO RUN:  ./build/demos 10
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

void demo_sugar_advanced(MT5Sugar& sugar, const Config& cfg) {
    const std::string sym = cfg.testSymbol;
    const double vol = cfg.testVolume;

    // ── 1. SYMBOL HELPERS ────────────────────────────────────────────────────
    printSub(std::string("Symbol helpers for ") + sym);

    // ──────────────────────────────────────────────────────────────────────────
    //  Quick symbol look-ups:
    //    isSymbolAvailable(sym) → bool     — exists and selectable
    //    getSymbolDigits(sym)   → int      — price decimal places
    //    getMinStopLevel(sym)   → int64_t  — min SL/TP distance, in points
    //    getAllSymbols()        → vector<string>  — every symbol name (count shown)
    // ──────────────────────────────────────────────────────────────────────────
    std::cout << "  available   : " << (sugar.isSymbolAvailable(sym) ? "yes" : "no") << "\n";
    std::cout << "  digits      : " << sugar.getSymbolDigits(sym) << "\n";
    std::cout << "  min stop lvl: " << sugar.getMinStopLevel(sym) << " points\n";
    std::cout << "  symbols on terminal: " << sugar.getAllSymbols().size() << "\n";

    // ── 2. RISK MANAGEMENT ───────────────────────────────────────────────────
    printSub("Risk management (no orders placed)");

    // ──────────────────────────────────────────────────────────────────────────
    //  calculateRequiredMargin(sym, vol) → double   — margin to open `vol` lots
    //  getMaxLotSize(sym)                → double   — biggest lot the free margin allows
    //  calculatePositionSize(sym, risk%, slPips) → double — lots for a % risk on a pip stop
    // ──────────────────────────────────────────────────────────────────────────
    std::cout << "  required margin for " << vol << " lots : " << sugar.calculateRequiredMargin(sym, vol) << "\n";
    std::cout << "  max lot size (free margin)        : " << sugar.getMaxLotSize(sym) << "\n";
    std::cout << "  size for 1% risk over 50-pip stop : " << sugar.calculatePositionSize(sym, 1.0, 50.0) << " lots\n";

    // ──────────────────────────────────────────────────────────────────────────
    //  canOpenPosition(sym, vol, reason&)
    //     Whether a `vol`-lot order can open now; `reason` explains a false.
    //     Returns: bool.
    // ──────────────────────────────────────────────────────────────────────────
    std::string reason;
    bool can = sugar.canOpenPosition(sym, vol, reason);
    std::cout << "  can open " << vol << " lots? " << (can ? "yes" : ("no — " + reason)) << "\n";

    // ──────────────────────────────────────────────────────────────────────────
    //  calculateSLTP(sym, dir, entry, slPips, tpPips, sl&, tp&)
    //     Absolute SL/TP prices from pip distances (rounded to the symbol's digits).
    // ──────────────────────────────────────────────────────────────────────────
    double entry = sugar.getAsk(sym), sl = 0, tp = 0;
    sugar.calculateSLTP(sym, "buy", entry, /*slPips*/ 30, /*tpPips*/ 60, sl, tp);
    std::cout << "  buy @ " << entry << "  ->  SL=" << sl << "  TP=" << tp
              << "  (30/60 pips)\n";
}

} // namespace demos
