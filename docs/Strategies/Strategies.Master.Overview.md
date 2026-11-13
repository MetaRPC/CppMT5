# Strategies — Master Overview

**Orchestrators** are small strategy managers built on top of [`MT5Sugar`](../MT5Sugar/MT5Sugar.API_Overview.md):
they wrap a decision loop (read state → act → repeat) around the one-liner API. A **preset**
coordinates orchestrators behind a single decision. All of them live in `examples/demos/` and run
from the `demos` launcher.

!!! warning "Educational examples, not production systems"
    These orchestrators use deliberately simple heuristics (a trailing offset, a fixed grid, a
    few account thresholds). They demonstrate the *pattern* — a strategy built from the SDK — and
    are templates to build on, not turnkey trading systems. They place small live orders on the
    **demo** account and clean up after themselves.

## The pattern

Each orchestrator is a class that:

1. takes a connected `MT5Sugar&` and a small `Config` in its constructor,
2. exposes a `run()` (or `execute()`) that loops for a bounded number of cycles,
3. acts through Sugar / Service calls and prints what it does.

```cpp
class RiskManager {
public:
    struct Config { double maxDrawdownPct{5}; double minMarginLevel{150}; int cycles{5}; };
    RiskManager(metarpc::MT5Sugar& sugar, Config cfg);
    void run();   // read metrics → evaluate limits → act
};
```

## Orchestrators

Each orchestrator has its **own page** with the annotated class and several worked examples:

| # | Orchestrator | What it does | Key Sugar calls |
|---|--------------|--------------|-----------------|
| **11** | **[Trailing Stop](Orchestrators/TrailingStop.md)** | Opens one position, then raises its stop loss as price advances (never lowers it); closes at the end. | `buyMarket`, `getBid`, `modifyPositionSL`, `closePosition` |
| **13** | **[Grid Trader](Orchestrators/GridTrader.md)** | Lays a symmetric grid of pending orders around price (`levels` each side, `stepPips` apart), lists it, then removes it. | `buyLimit`, `sellLimit`, `service().openedOrders`, `closePosition` |
| **14** | **[Risk Manager](Orchestrators/RiskManager.md)** | Monitors account health each cycle; if drawdown or margin level breaches a limit, it closes everything (kill-switch). | `getEquity`, `getMarginLevel`, `getTotalProfit`, `closeAllPositions` |

### [Trailing Stop →](Orchestrators/TrailingStop.md)

Ride winners, cap the downside. Each cycle reads the bid and, if price advanced, tightens the SL to `trailPips` behind it — never looser.

```cpp
double wantSL = bid - trailPips * pip;
if (wantSL > bestSL) { bestSL = wantSL; sugar.modifyPositionSL(ticket, bestSL); }
```

### [Grid Trader →](Orchestrators/GridTrader.md)

Fade a range: buy-limits below price, sell-limits above, `levels` each side.

```cpp
for (int i = 1; i <= levels; ++i) {
    sugar.buyLimit (sym, vol, bid - i * stepPips * pip);   // below price
    sugar.sellLimit(sym, vol, ask + i * stepPips * pip);   // above price
}
// … later: sugar.closePosition(ticket) deletes each pending order.
```

### [Risk Manager →](Orchestrators/RiskManager.md)

A kill-switch: monitor account health and flatten everything on a breach.

```cpp
double ddPct = (startBalance - sugar.getEquity()) / startBalance * 100.0;
if (ddPct > maxDrawdownPct || (open > 0 && marginLevel > 0 && marginLevel < minMarginLevel))
    sugar.closeAllPositions();   // kill-switch
```

## Preset

| # | Preset | What it does |
|---|--------|--------------|
| **16** | **[Adaptive Market Mode](Presets/AdaptiveMarketModePreset.md)** | Analyses recent bars, classifies the market (Trending / Ranging / Volatile) and dispatches to the matching orchestrator. |

```cpp
// analyse last 20 H1 bars → pick a mode → run the right orchestrator
if      (spike > 2.5) demo_risk_manager(sugar, cfg);     // VOLATILE  → defensive
else if (trend > 2.0) demo_trailing_stop(sugar, cfg);    // TRENDING  → ride it
else                  demo_grid_trader(sugar, cfg);      // RANGING   → fade it
```

## Running them

```bash
./build/demos 11    # trailing stop
./build/demos 13    # grid trader
./build/demos 14    # risk manager
./build/demos 16    # adaptive preset
```
