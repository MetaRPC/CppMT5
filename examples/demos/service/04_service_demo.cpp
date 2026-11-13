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

    // ──────────────────────────────────────────────────────────────────────────
    //  1  accountSummary()
    //     Whole account snapshot as a friendly struct (times already UTC).
    //     Returns: AccountInfo (login, company, balance, currency, leverage…).
    // ──────────────────────────────────────────────────────────────────────────
    AccountInfo a = svc.accountSummary();
    std::cout << "  account : #" << a.login << "  " << a.company
              << "  bal=" << a.balance << " " << a.currency << "  1:" << a.leverage << "\n";

    // ──────────────────────────────────────────────────────────────────────────
    //  2  symbolInfoTick(symbol)
    //     Latest quote as a friendly struct.
    //     Returns: Tick (symbol, bid, ask, last, volume, UTC time; twoSided()).
    // ──────────────────────────────────────────────────────────────────────────
    Tick t = svc.symbolInfoTick(sym);
    std::cout << "  tick    : " << t.symbol << "  bid=" << t.bid << " ask=" << t.ask
              << "  (two-sided: " << (t.twoSided() ? "yes" : "no") << ")\n";

    // ──────────────────────────────────────────────────────────────────────────
    //  3  symbolParams(symbol)
    //     Full symbol specification in one call (prefer over many single reads).
    //     Returns: SymbolInfo (digits, point, volumeMin/Max/Step, tickValue…).
    // ──────────────────────────────────────────────────────────────────────────
    SymbolInfo si = svc.symbolParams(sym);
    std::cout << "  params  : digits=" << si.digits << " point=" << si.point
              << " vol[min=" << si.volumeMin << " max=" << si.volumeMax << " step=" << si.volumeStep << "]\n";

    // ──────────────────────────────────────────────────────────────────────────
    //  4  symbolInfoMarginRate(symbol, orderType)
    //     Initial/maintenance margin-rate multipliers for a symbol + direction.
    //     Returns: MarginRate (initial, maintenance).
    // ──────────────────────────────────────────────────────────────────────────
    MarginRate mr = svc.symbolInfoMarginRate(sym, /*ENUM_ORDER_TYPE buy*/ 0);
    std::cout << "  margin  : initial=" << mr.initial << " maintenance=" << mr.maintenance << "\n";

    // ── 2. POSITIONS / ORDERS ────────────────────────────────────────────────
    printSub("Positions / orders");

    // ──────────────────────────────────────────────────────────────────────────
    //  5  positionsTotal()
    //     How many positions are open right now.
    //     Returns: int.
    // ──────────────────────────────────────────────────────────────────────────
    std::cout << "  open positions: " << svc.positionsTotal() << "\n";

    // ──────────────────────────────────────────────────────────────────────────
    //  6  openedOrders()
    //     Everything live: open positions AND pending orders, as friendly structs.
    //     Returns: OpenedOrders { positions[], orders[] }.
    // ──────────────────────────────────────────────────────────────────────────
    OpenedOrders oo = svc.openedOrders();
    std::cout << "  snapshot: " << oo.positions.size() << " positions, "
              << oo.orders.size() << " pending orders\n";
    for (const auto& p : oo.positions)
        std::cout << "    #" << p.ticket << "  " << p.symbol << "  "
                  << (p.isBuy() ? "BUY " : "SELL") << "  vol=" << p.volume << "  profit=" << p.profit << "\n";

    // ── 3. BARS ──────────────────────────────────────────────────────────────
    printSub(std::string("Last 5 H1 bars of ") + sym);

    // ──────────────────────────────────────────────────────────────────────────
    //  7  lastBars(symbol, timeframe, count)
    //     The newest N OHLC bars for a symbol/timeframe (UTC bar times).
    //     Returns: std::vector<Bar> (time, open, high, low, close, tickVolume…).
    // ──────────────────────────────────────────────────────────────────────────
    auto bars = svc.lastBars(sym, Timeframe::H1, 5);
    for (const auto& b : bars)
        std::cout << "    t=" << b.time << "  O=" << b.open << " H=" << b.high
                  << " L=" << b.low << " C=" << b.close << "  vol=" << b.tickVolume << "\n";
}

} // namespace demos
