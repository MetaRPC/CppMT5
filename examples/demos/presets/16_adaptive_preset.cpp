/*══════════════════════════════════════════════════════════════════════════════
 FILE: 16_adaptive_preset.cpp - PRESET: ADAPTIVE MARKET MODE

 PURPOSE:
   A "preset" coordinates orchestrators behind one decision. This one reads recent
   price action, classifies the market, and dispatches to the matching orchestrator:

       TRENDING  -> Trailing Stop   (ride the move, trail the stop)
       RANGING   -> Grid Trader     (fade the range with a pending grid)
       VOLATILE  -> Risk Manager    (defensive monitoring / kill-switch)

 📚 PATTERN:
   analyse last N bars (avg range = volatility, close delta = trend) → pick a mode →
   run the chosen orchestrator. Built on MT5Service (bars) + the demo orchestrators.

 🔄 API LEVELS:  MID (bars) + HIGH (orchestrators).   🚀 HOW TO RUN:  ./build/demos 16
══════════════════════════════════════════════════════════════════════════════*/

#include <cmath>
#include <iostream>
#include <string>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"
#include "demos.hpp"

namespace demos {

using namespace metarpc;

namespace {
enum class MarketMode { Trending, Ranging, Volatile };
const char* name(MarketMode m) {
    switch (m) { case MarketMode::Trending: return "TRENDING";
                 case MarketMode::Ranging:  return "RANGING";
                 default:                    return "VOLATILE"; }
}
}

class AdaptiveMarketModePreset {
public:
    AdaptiveMarketModePreset(MT5Sugar& sugar, std::string symbol)
        : m_s(sugar), m_sym(std::move(symbol)) {}

    MarketMode analyse() {
        // ──────────────────────────────────────────────────────────────────────
        //  1  service().lastBars(symbol, timeframe, count)   — drop to mid-level
        //     Pull recent OHLC bars to classify the market (volatility vs trend).
        //     Returns: std::vector<Bar> (time, open, high, low, close…).
        // ──────────────────────────────────────────────────────────────────────
        auto bars = m_s.service().lastBars(m_sym, Timeframe::H1, 20);
        if (bars.size() < 5) { std::cout << "  not enough bars — defaulting to RANGING\n"; return MarketMode::Ranging; }

        double sumRange = 0, maxRange = 0;
        for (const auto& b : bars) { double r = b.high - b.low; sumRange += r; maxRange = std::max(maxRange, r); }
        double avgRange = sumRange / bars.size();
        double move = std::fabs(bars.back().close - bars.front().close);  // net directional move
        double trend = avgRange > 0 ? move / avgRange : 0;                // in "average ranges"
        double spike = avgRange > 0 ? maxRange / avgRange : 1;            // biggest bar vs typical

        std::cout << "  analysed " << bars.size() << " H1 bars:  avgRange=" << avgRange
                  << "  netMove=" << move << " (" << trend << " ranges)  maxSpike=" << spike << "x\n";

        if (spike > 2.5)      return MarketMode::Volatile;   // an outsized bar → defensive
        if (trend > 2.0)      return MarketMode::Trending;   // strong directional drift
        return MarketMode::Ranging;
    }

    void execute(const Config& cfg) {
        MarketMode mode = analyse();
        std::cout << "  → market mode: " << name(mode) << "  → dispatching orchestrator\n\n";
        // ──────────────────────────────────────────────────────────────────────
        //  2  dispatch to the matching orchestrator (each runs its own API calls):
        //     TRENDING → demo_trailing_stop   RANGING → demo_grid_trader
        //     VOLATILE → demo_risk_manager
        // ──────────────────────────────────────────────────────────────────────
        switch (mode) {
            case MarketMode::Trending: demo_trailing_stop(m_s, cfg); break;
            case MarketMode::Ranging:  demo_grid_trader(m_s, cfg);   break;
            case MarketMode::Volatile: demo_risk_manager(m_s, cfg);  break;
        }
    }

private:
    MT5Sugar& m_s;
    std::string m_sym;
};

void demo_adaptive_preset(MT5Sugar& sugar, const Config& cfg) {
    printSub("Adaptive market-mode preset (analyse → pick → run an orchestrator)");
    AdaptiveMarketModePreset preset(sugar, cfg.testSymbol);
    preset.execute(cfg);
}

} // namespace demos
