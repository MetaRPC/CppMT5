/*══════════════════════════════════════════════════════════════════════════════
 FILE: 04_service_demo.cpp - MID-LEVEL MT5Service FACADE

 PURPOSE:
   The comfortable middle layer: plain arguments in, friendly C++ structs out, with
   UTC time conversion done for you. No protobuf, far less code than the low level.

 📚 WHAT THIS DEMO COVERS (3 categories):

   1. ACCOUNT & SYMBOLS
      • accountSummary()   -> AccountInfo
      • symbolInfoTick()   -> Tick
      • symbolParams()     -> SymbolInfo (digits/point/volume limits/…)
      • symbolInfoMarginRate() -> MarginRate

   2. POSITIONS / ORDERS
      • positionsTotal()   -> int
      • openedOrders()     -> OpenedOrders (positions + pendings)

   3. BARS
      • lastBars()         -> newest N OHLC bars

 🔄 API LEVELS:
   MID-LEVEL (Service) - THIS FILE: friendly structs, UTC times
   LOW-LEVEL (Account) - raw protobuf                         → sugar.account()
   HIGH-LEVEL (Sugar)  - one-liners                           → sugar

 🚀 HOW TO RUN:  ./build/demos 4
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

void demo_service_demo(MT5Sugar& sugar, const Config& cfg) {
    MT5Service& svc = sugar.service();   // the mid-level layer
    const std::string sym = cfg.testSymbol;

    // ── 1. ACCOUNT & SYMBOLS ─────────────────────────────────────────────────
    printSub("Account & symbols");
    AccountInfo a = svc.accountSummary();
    std::cout << "  account : #" << a.login << "  " << a.company
              << "  bal=" << a.balance << " " << a.currency << "  1:" << a.leverage << "\n";

    Tick t = svc.symbolInfoTick(sym);
    std::cout << "  tick    : " << t.symbol << "  bid=" << t.bid << " ask=" << t.ask
              << "  (two-sided: " << (t.twoSided() ? "yes" : "no") << ")\n";

    SymbolInfo si = svc.symbolParams(sym);
    std::cout << "  params  : digits=" << si.digits << " point=" << si.point
              << " vol[min=" << si.volumeMin << " max=" << si.volumeMax << " step=" << si.volumeStep << "]\n";

    MarginRate mr = svc.symbolInfoMarginRate(sym, /*ENUM_ORDER_TYPE buy*/ 0);
    std::cout << "  margin  : initial=" << mr.initial << " maintenance=" << mr.maintenance << "\n";

    // ── 2. POSITIONS / ORDERS ────────────────────────────────────────────────
    printSub("Positions / orders");
    std::cout << "  open positions: " << svc.positionsTotal() << "\n";
    OpenedOrders oo = svc.openedOrders();
    std::cout << "  snapshot: " << oo.positions.size() << " positions, "
              << oo.orders.size() << " pending orders\n";
    for (const auto& p : oo.positions)
        std::cout << "    #" << p.ticket << "  " << p.symbol << "  "
                  << (p.isBuy() ? "BUY " : "SELL") << "  vol=" << p.volume << "  profit=" << p.profit << "\n";

    // ── 3. BARS ──────────────────────────────────────────────────────────────
    printSub(std::string("Last 5 H1 bars of ") + sym);
    auto bars = svc.lastBars(sym, Timeframe::H1, 5);
    for (const auto& b : bars)
        std::cout << "    t=" << b.time << "  O=" << b.open << " H=" << b.high
                  << " L=" << b.low << " C=" << b.close << "  vol=" << b.tickVolume << "\n";
}

} // namespace demos
