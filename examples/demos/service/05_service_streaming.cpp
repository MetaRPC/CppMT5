/*══════════════════════════════════════════════════════════════════════════════
 FILE: 05_service_streaming.cpp - MID-LEVEL MT5Service STREAMING

 PURPOSE:
   The friendly streaming layer: subscribe once, receive ready-to-use structs (Tick,
   TradeEvent, …) with times already in UTC. No protobuf in sight.

 📚 WHAT THIS DEMO COVERS:
   • subscribeTicks()      -> friendly Tick stream (~5s)
   • streamTradeUpdates()  -> TradeEvent stream (watches for any trade activity, ~3s)

 🔄 API LEVELS:
   MID-LEVEL (Service) - THIS FILE: friendly Tick / TradeEvent
   LOW-LEVEL (Account) - raw pb payloads                     → sugar.account().onSymbolTick()

 🚀 HOW TO RUN:  ./build/demos 5
══════════════════════════════════════════════════════════════════════════════*/

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

void demo_service_streaming(MT5Sugar& sugar, const Config& cfg) {
    MT5Service& svc = sugar.service();
    const std::string sym = cfg.testSymbol;

    // ──────────────────────────────────────────────────────────────────────────
    //  0  symbolSelect(symbol, select)  — ensure the symbol streams
    //     Returns: bool.
    // ──────────────────────────────────────────────────────────────────────────
    svc.symbolSelect(sym, true);

    // ── Friendly tick stream ─────────────────────────────────────────────────
    printSub(std::string("Friendly Tick stream for ") + sym + " (~5s)");

    // ──────────────────────────────────────────────────────────────────────────
    //  1  subscribeTicks(symbols, callback)
    //     Live ticks delivered as friendly Tick structs (UTC) on a background thread.
    //     Returns: std::unique_ptr<ServiceStream> — keep it alive; stop() to end.
    // ──────────────────────────────────────────────────────────────────────────
    std::atomic<int> n{0};
    auto ticks = svc.subscribeTicks({sym}, [&](const Tick& t) {
        int i = ++n;
        if (i <= 10)
            std::cout << "  tick #" << i << "  " << t.symbol << "  bid=" << t.bid
                      << " ask=" << t.ask << "  t=" << t.time << " (UTC)\n";
    });
    for (int i = 0; i < 50 && ticks->running(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ticks->stop();
    std::cout << "  received " << ticks->count() << " ticks\n";

    // ── Trade-event stream ───────────────────────────────────────────────────
    printSub("Trade-event stream (~3s; fires only on account activity)");

    // ──────────────────────────────────────────────────────────────────────────
    //  2  streamTradeUpdates(callback)
    //     Push notifications when positions/orders/deals appear, change or close.
    //     Returns: std::unique_ptr<ServiceStream>; callback gets a friendly TradeEvent.
    // ──────────────────────────────────────────────────────────────────────────
    std::atomic<int> ev{0};
    auto trades = svc.streamTradeUpdates([&](const TradeEvent& e) {
        ++ev;
        std::cout << "  trade event: type=" << e.type
                  << "  new positions=" << e.newPositions.size()
                  << "  updated=" << e.updatedPositions.size()
                  << "  balance=" << e.account.balance << "\n";
    });
    std::this_thread::sleep_for(std::chrono::seconds(3));
    trades->stop();
    std::cout << "  trade events seen: " << ev.load()
              << (ev.load() == 0 ? "  (no account activity during the window — expected)\n" : "\n");
}

} // namespace demos
