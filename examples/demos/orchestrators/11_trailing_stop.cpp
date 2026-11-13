/*══════════════════════════════════════════════════════════════════════════════
 FILE: 11_trailing_stop.cpp - ORCHESTRATOR: TRAILING STOP  (⚠ places a demo order)

 PURPOSE:
   Opens one small BUY position and then trails its stop loss: each cycle, if price
   has advanced, the SL is raised to lock in gains (never lowered). Closes the
   position at the end. The classic "let winners run, cap the downside" manager.

 📚 PATTERN:
   TrailingStop{config} → open → run(cycles): read price → if profit grew by >= step,
   raise SL → repeat → close. Built on MT5Sugar (buyMarket / getBid /
   getPositionByTicket / modifyPositionSL / closePosition).

 ⚠ SAFETY: runs a live 0.01 trade on the demo account and closes it before returning.

 🔄 API LEVELS:  HIGH-LEVEL (Sugar).   🚀 HOW TO RUN:  ./build/demos 11
══════════════════════════════════════════════════════════════════════════════*/

#include <chrono>
#include <iostream>
#include <thread>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

class TrailingStop {
public:
    struct Config {
        double trailPips{20.0};   // keep the SL this many pips behind the best bid
        int    cycles{8};
        int    intervalMs{700};
    };

    TrailingStop(MT5Sugar& sugar, std::string symbol, double volume, Config cfg)
        : m_s(sugar), m_sym(std::move(symbol)), m_vol(volume), m_cfg(cfg) {
        m_pip = sugar.getSymbolInfo(m_sym).point;
        if (sugar.getSymbolDigits(m_sym) == 3 || sugar.getSymbolDigits(m_sym) == 5) m_pip *= 10.0;
    }

    void run() {
        uint64_t ticket = m_s.buyMarket(m_sym, m_vol);
        if (ticket == 0) { std::cout << "  order rejected (market closed?) — nothing to trail\n"; return; }
        std::cout << "  opened #" << ticket << " BUY " << m_vol << " " << m_sym << "\n";

        double bestSL = 0;
        for (int c = 1; c <= m_cfg.cycles; ++c) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_cfg.intervalMs));
            bool found = false;
            Position p = m_s.getPositionByTicket((int64_t)ticket, found);
            if (!found) { std::cout << "  position gone (closed elsewhere)\n"; return; }

            double bid = m_s.getBid(m_sym);
            double wantSL = bid - m_cfg.trailPips * m_pip;     // SL trails trailPips behind bid
            if (wantSL > bestSL) {                             // only ever tighten
                bestSL = wantSL;
                bool ok = m_s.modifyPositionSL((int64_t)ticket, bestSL);
                std::cout << "  cycle " << c << "  bid=" << bid << "  raise SL -> " << bestSL
                          << (ok ? "  ✓" : "  (modify failed)") << "\n";
            } else {
                std::cout << "  cycle " << c << "  bid=" << bid << "  SL held at " << bestSL << "\n";
            }
        }
        std::cout << "  closing #" << ticket << " -> " << (m_s.closePosition((int64_t)ticket) ? "closed" : "FAILED") << "\n";
    }

private:
    MT5Sugar& m_s;
    std::string m_sym;
    double m_vol, m_pip{0};
    Config m_cfg;
};

void demo_trailing_stop(MT5Sugar& sugar, const Config& cfg) {
    printSub("Trailing stop (open, trail SL, close)");
    TrailingStop ts(sugar, cfg.testSymbol, cfg.testVolume, TrailingStop::Config{});
    ts.run();
}

} // namespace demos
