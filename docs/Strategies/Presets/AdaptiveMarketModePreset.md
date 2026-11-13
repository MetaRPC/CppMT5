# Adaptive Market-Mode Preset

A **preset** makes one decision and delegates the work to an orchestrator. The adaptive
market-mode preset reads recent price action, classifies the market, and dispatches to the
orchestrator that fits:

| Market mode | Chosen orchestrator | Rationale |
|-------------|--------------------|-----------|
| **TRENDING** | [Trailing Stop](../Orchestrators/TrailingStop.md) | ride the move, trail the stop |
| **RANGING** | [Grid Trader](../Orchestrators/GridTrader.md) | fade the range with a pending grid |
| **VOLATILE** | [Risk Manager](../Orchestrators/RiskManager.md) | defensive monitoring / kill-switch |

Source: `examples/demos/presets/16_adaptive_preset.cpp` — run with `./build/demos 16`.

## How it analyses the market

It pulls the last 20 H1 bars through `MT5Service::lastBars` and derives three cheap numbers:

```cpp
auto bars = sugar.service().lastBars(symbol, Timeframe::H1, 20);

double sumRange = 0, maxRange = 0;
for (const auto& b : bars) { double r = b.high - b.low; sumRange += r; maxRange = std::max(maxRange, r); }
double avgRange = sumRange / bars.size();                 // typical bar size  → volatility
double move     = std::fabs(bars.back().close - bars.front().close);
double trend    = avgRange > 0 ? move / avgRange : 0;     // net move in "average ranges"
double spike    = avgRange > 0 ? maxRange / avgRange : 1; // biggest bar vs typical
```

## How it decides

```cpp
if      (spike > 2.5) mode = VOLATILE;   // an outsized bar → something is happening, be defensive
else if (trend > 2.0) mode = TRENDING;   // price drifted several ranges in one direction
else                  mode = RANGING;    // quiet, mean-reverting

switch (mode) {
    case TRENDING: demo_trailing_stop(sugar, cfg); break;
    case RANGING:  demo_grid_trader(sugar, cfg);   break;
    case VOLATILE: demo_risk_manager(sugar, cfg);  break;
}
```

## Example run

```
analysed 20 H1 bars:  avgRange=0.0009075  netMove=0.0003 (0.33 ranges)  maxSpike=1.63x
→ market mode: RANGING  → dispatching orchestrator
   (Grid Trader places a grid, lists it, removes it)
```

## 🔗 Usage Examples

### 1) Run it

```cpp
MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "password", "MetaQuotes-Demo");

AdaptiveMarketModePreset preset(sugar, "EURUSD");
preset.execute(cfg);   // analyse → classify → dispatch to one orchestrator
```

### 2) Tune the classifier window / thresholds

The analysis is a few lines — fork it for a different regime definition:

```cpp
auto bars = sugar.service().lastBars("EURUSD", Timeframe::M15, 50);   // finer, more bars
// ... recompute avgRange / trend / spike, then use your own thresholds ...
if      (spike > 3.0) mode = VOLATILE;   // more tolerant of spikes
else if (trend > 1.5) mode = TRENDING;   // quicker to call a trend
else                  mode = RANGING;
```

### 3) Classify without trading (regime signal only)

Use the analysis as a signal you feed elsewhere, instead of dispatching:

```cpp
AdaptiveMarketModePreset preset(sugar, "EURUSD");
MarketMode mode = preset.analyse();                 // just the classification
std::printf("regime: %s\n", name(mode));
// → route to your own logic, size differently per regime, etc.
```

!!! note
    The thresholds (`2.5` spike, `2.0` trend) and the H1/20-bar window are deliberately simple and
    meant to be tuned. The value of the preset is the *shape*: analyse → classify → dispatch — a
    clean place to plug in real indicators or an economic calendar.

---

## 📚 Related

* [Strategies Overview](../Strategies.Master.Overview.md)
* Orchestrators it dispatches to: [Trailing Stop](../Orchestrators/TrailingStop.md) · [Grid Trader](../Orchestrators/GridTrader.md) · [Risk Manager](../Orchestrators/RiskManager.md)
* Mid-level bars: [MT5Service Symbol Methods](../../MT5Service/2.%20Symbol_Methods.md) (`lastBars`)
