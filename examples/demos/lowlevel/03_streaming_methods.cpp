/*══════════════════════════════════════════════════════════════════════════════
 FILE: 03_streaming_methods.cpp - LOW-LEVEL MT5Account STREAMING

 PURPOSE:
   Shows the raw server-stream pattern: open a subscription, receive protobuf
   payloads on a background thread, and stop by letting the handle go out of scope.

 📚 WHAT THIS DEMO COVERS:
   • onSymbolTick() - live ticks (pb::OnSymbolTickData) for a few seconds
   • AccountStream  - running()/count()/lastError()/stop()

 🔄 API LEVELS:
   LOW-LEVEL (Account) - THIS FILE: raw pb::OnSymbolTickData payloads
   HIGH-LEVEL (Service)- friendly Tick objects              → sugar.service().subscribeTicks()

 🚀 HOW TO RUN:  ./build/demos 3
══════════════════════════════════════════════════════════════════════════════*/

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

namespace pb = mt5_term_api;   // protobuf types for the raw layer

void demo_streaming_methods(metarpc::MT5Sugar& sugar, const Config& cfg) {
    metarpc::MT5Account& acc = sugar.account();
    const std::string sym = cfg.testSymbol;

    printSub(std::string("Streaming ticks for ") + sym + " (~5s)");

    // ──────────────────────────────────────────────────────────────────────────
    //  1  symbolSelect(symbol, select)  — ticks only flow for selected symbols
    //     Make sure the symbol is in Market Watch before subscribing.
    //     Returns: SymbolSelectData → success().
    // ──────────────────────────────────────────────────────────────────────────
    pb::SymbolSelectRequest sel; sel.set_symbol(sym); sel.set_select(true);
    acc.symbolSelect(sel);

    // ──────────────────────────────────────────────────────────────────────────
    //  2  onSymbolTick(request, callback)  — open the live tick stream
    //     The SDK reads the server stream on a BACKGROUND thread and calls the
    //     lambda for every pb::OnSymbolTickData. Keep the returned handle alive.
    //     Returns: std::unique_ptr<AccountStream>.
    // ──────────────────────────────────────────────────────────────────────────
    std::atomic<int> n{0};
    pb::OnSymbolTickRequest req; req.add_symbol_names(sym);
    auto stream = acc.onSymbolTick(req, [&](const pb::OnSymbolTickData& d) {
        if (!d.has_symbol_tick()) return;
        const auto& t = d.symbol_tick();
        int i = ++n;
        if (i <= 10)  // print the first few
            std::cout << "  tick #" << i << "  " << t.symbol()
                      << "  bid=" << t.bid() << "  ask=" << t.ask() << "\n";
    });

    // ──────────────────────────────────────────────────────────────────────────
    //  3  AccountStream handle — running() / count() / lastError() / stop()
    //     Let it run ~5 s, then stop() cancels the stream and joins the thread.
    // ──────────────────────────────────────────────────────────────────────────
    for (int i = 0; i < 50 && stream->running(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    stream->stop();
    std::cout << "  received " << stream->count() << " ticks\n";
    if (!stream->lastError().empty())
        std::cout << "  stream ended with: " << stream->lastError() << "\n";
}

} // namespace demos
