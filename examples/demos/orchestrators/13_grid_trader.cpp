/*══════════════════════════════════════════════════════════════════════════════
 FILE: 13_grid_trader.cpp - ORCHESTRATOR: GRID TRADER  (⚠ places demo pending orders)

 PURPOSE:
   Lays a symmetric grid of pending orders around the current price: buy-limits below
   and sell-limits above, `levels` each side, `stepPips` apart. Lists what it placed,
   then cleans the whole grid up again. Demonstrates pending-order placement + removal.

 📚 PATTERN:
   GridTrader{config} → place grid (buyLimit / sellLimit) → list (openedOrders) →
   remove (closePosition deletes a pending by ticket). Built on MT5Sugar + service().

 ⚠ SAFETY: places small live pending orders on the demo account and deletes them all
   before returning. Nothing is left open.

 🔄 API LEVELS:  HIGH-LEVEL (Sugar) + service().   🚀 HOW TO RUN:  ./build/demos 13
══════════════════════════════════════════════════════════════════════════════*/

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

class GridTrader {
public:
    struct Config {
        int    levels{2};        // pending orders on each side
        double stepPips{20.0};   // distance between levels
        double volume{0.01};
    };

    GridTrader(MT5Sugar& sugar, std::string symbol, Config cfg)
        : m_s(sugar), m_sym(std::move(symbol)), m_cfg(cfg) {
        m_pip = sugar.getSymbolInfo(m_sym).point;
        if (sugar.getSymbolDigits(m_sym) == 3 || sugar.getSymbolDigits(m_sym) == 5) m_pip *= 10.0;
    }

    void run() {
        // ──────────────────────────────────────────────────────────────────────
        //  1  getBid(symbol) / getAsk(symbol) → double   — centre the grid on price
        // ──────────────────────────────────────────────────────────────────────
        double bid = m_s.getBid(m_sym), ask = m_s.getAsk(m_sym);
        std::cout << "  grid around bid=" << bid << " ask=" << ask
                  << "  (" << m_cfg.levels << " levels/side, " << m_cfg.stepPips << " pips step)\n";

        // ── place the grid ───────────────────────────────────────────────────
        // ──────────────────────────────────────────────────────────────────────
        //  2  buyLimit(symbol, volume, price)  / sellLimit(...)   ⚠ REAL pending orders
        //     A buy-limit `i` steps below and a sell-limit `i` steps above, per level.
        //     Returns: uint64_t ticket each (0 if rejected).
        // ──────────────────────────────────────────────────────────────────────
        std::vector<uint64_t> tickets;
        for (int i = 1; i <= m_cfg.levels; ++i) {
            double below = bid - i * m_cfg.stepPips * m_pip;    // buy-limit under price
            double above = ask + i * m_cfg.stepPips * m_pip;    // sell-limit over price
            uint64_t b = m_s.buyLimit(m_sym, m_cfg.volume, below);
            uint64_t s = m_s.sellLimit(m_sym, m_cfg.volume, above);
            if (b) { tickets.push_back(b); std::cout << "    buyLimit  @ " << below << "  #" << b << "\n"; }
            if (s) { tickets.push_back(s); std::cout << "    sellLimit @ " << above << "  #" << s << "\n"; }
        }
        if (tickets.empty()) { std::cout << "  no orders placed (market closed / too close to price?)\n"; return; }

        // ── show the live pending orders ─────────────────────────────────────
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        // ──────────────────────────────────────────────────────────────────────
        //  3  service().openedOrders().orders   — drop to mid-level for the list
        //     Returns: std::vector<PendingOrder> (the pending orders now live).
        // ──────────────────────────────────────────────────────────────────────
        auto pendings = m_s.service().openedOrders().orders;
        std::cout << "  pending orders now live: " << pendings.size() << "\n";

        // ── clean up: delete everything we placed ────────────────────────────
        // ──────────────────────────────────────────────────────────────────────
        //  4  closePosition(ticket)   ⚠ deletes a pending order by ticket
        //     (same call closes a position; here each ticket is a pending order).
        // ──────────────────────────────────────────────────────────────────────
        int removed = 0;
        for (uint64_t t : tickets) if (m_s.closePosition((int64_t)t)) ++removed;
        std::cout << "  removed " << removed << "/" << tickets.size() << " grid orders\n";
    }

private:
    MT5Sugar& m_s;
    std::string m_sym;
    double m_pip{0};
    Config m_cfg;
};

void demo_grid_trader(MT5Sugar& sugar, const Config& cfg) {
    printSub("Grid trader (place a grid, list it, remove it)");
    GridTrader gt(sugar, cfg.testSymbol, GridTrader::Config{2, 20.0, cfg.testVolume});
    gt.run();
}

} // namespace demos
