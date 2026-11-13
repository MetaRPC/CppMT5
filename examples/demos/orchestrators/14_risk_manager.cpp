/*══════════════════════════════════════════════════════════════════════════════
 FILE: 14_risk_manager.cpp - ORCHESTRATOR: RISK MANAGER

 PURPOSE:
   A monitoring orchestrator: every cycle it reads account health (margin level,
   equity, floating P/L, open positions) and enforces guard rails — if drawdown
   exceeds maxDrawdownPct or the margin level drops below minMarginLevel, it closes
   every position (a protective kill-switch). On a healthy/flat account it just reports.

 📚 PATTERN:
   RiskManager{config} → run(cycles): read metrics → evaluate limits → act → repeat.
   Built entirely on MT5Sugar (getMarginLevel / getEquity / getTotalProfit /
   countOpenPositions / closeAllPositions).

 🔄 API LEVELS:  HIGH-LEVEL (Sugar).   🚀 HOW TO RUN:  ./build/demos 14
══════════════════════════════════════════════════════════════════════════════*/

#include <chrono>
#include <iostream>
#include <thread>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

class RiskManager {
public:
    struct Config {
        double maxDrawdownPct{5.0};    // close everything if equity is this % below the start balance
        double minMarginLevel{150.0};  // ... or if the margin level drops below this %
        int    cycles{5};
        int    intervalMs{800};
    };

    RiskManager(MT5Sugar& sugar, Config cfg) : m_s(sugar), m_cfg(cfg) {
        m_startBalance = sugar.getBalance();
    }

    void run() {
        std::cout << "  start balance: " << m_startBalance
                  << "  | limits: drawdown <= " << m_cfg.maxDrawdownPct
                  << "%  margin level >= " << m_cfg.minMarginLevel << "%\n";
        for (int c = 1; c <= m_cfg.cycles; ++c) {
            // ──────────────────────────────────────────────────────────────────
            //  Account-health reads, one per cycle (all read-only):
            //    getEquity()         → double  — balance + floating P&L
            //    getMarginLevel()    → double  — percent (0 when flat)
            //    getTotalProfit()    → double  — summed floating P&L
            //    countOpenPositions()→ int     — how many positions are open
            // ──────────────────────────────────────────────────────────────────
            double equity = m_s.getEquity();
            double marginLevel = m_s.getMarginLevel();   // 0 when no margin is used
            double floating = m_s.getTotalProfit();
            int open = m_s.countOpenPositions();
            double ddPct = m_startBalance > 0 ? (m_startBalance - equity) / m_startBalance * 100.0 : 0.0;

            std::cout << "  cycle " << c << "/" << m_cfg.cycles
                      << "  equity=" << equity << "  floatP/L=" << floating
                      << "  marginLevel=" << marginLevel << "%  open=" << open
                      << "  drawdown=" << ddPct << "%\n";

            bool breach = (ddPct > m_cfg.maxDrawdownPct) ||
                          (open > 0 && marginLevel > 0 && marginLevel < m_cfg.minMarginLevel);
            if (breach) {
                // ──────────────────────────────────────────────────────────────
                //  closeAllPositions()   ⚠ kill-switch: flattens EVERY position
                //     Returns: int (how many were closed).
                // ──────────────────────────────────────────────────────────────
                int closed = m_s.closeAllPositions();
                std::cout << "  ⚠ LIMIT BREACHED — closed " << closed << " positions (kill-switch)\n";
                ++m_breaches;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(m_cfg.intervalMs));
        }
        std::cout << "  done. breaches handled: " << m_breaches << "\n";
    }

private:
    MT5Sugar& m_s;
    Config m_cfg;
    double m_startBalance{0};
    int m_breaches{0};
};

void demo_risk_manager(MT5Sugar& sugar, const Config& /*cfg*/) {
    printSub("Risk manager (monitor account health, kill-switch on breach)");
    RiskManager rm(sugar, RiskManager::Config{});
    rm.run();
}

} // namespace demos
