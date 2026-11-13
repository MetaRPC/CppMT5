# Orchestrator · Risk Manager

> A **monitoring** orchestrator: each cycle it reads account health and enforces guard rails — if drawdown or margin level breaches a limit, it **closes everything** (a kill-switch). Built entirely on [`MT5Sugar`](../../MT5Sugar/MT5Sugar.API_Overview.md).

**Source:** `examples/demos/orchestrators/14_risk_manager.cpp` · **Run:** `./build/demos 14` · **API level:** HIGH (Sugar)

!!! note "Read-only until a breach"
    Unlike the other orchestrators, this one **opens nothing**. It only monitors and, on a breach, flattens the book. Safe to run at any time.

---

## 🎯 What it does

1. Records the start balance.
2. Each cycle reads equity, margin level, floating P/L and open-position count.
3. Computes drawdown `% = (startBalance − equity) / startBalance × 100`.
4. If drawdown > `maxDrawdownPct`, **or** (with positions open) margin level < `minMarginLevel` → `closeAllPositions()` and stop.

## 🧩 The pattern

```
RiskManager{config} → run(cycles):
    read equity / marginLevel / floatP&L / openCount → evaluate limits
    → breach? closeAllPositions + stop : wait → repeat
```

## ⚙️ Config

```cpp
struct Config {
    double maxDrawdownPct{5.0};    // close all if equity is this % below the start balance
    double minMarginLevel{150.0};  // ... or if margin level drops below this % (with positions open)
    int    cycles{5};
    int    intervalMs{800};
};
```

---

## 🏗️ The orchestrator (annotated)

```cpp
class RiskManager {
public:
    struct Config { double maxDrawdownPct{5.0}; double minMarginLevel{150.0};
                    int cycles{5}; int intervalMs{800}; };

    RiskManager(MT5Sugar& sugar, Config cfg) : m_s(sugar), m_cfg(cfg) {
        m_startBalance = sugar.getBalance();      // baseline for drawdown
    }

    void run() {
        for (int c = 1; c <= m_cfg.cycles; ++c) {
            double equity      = m_s.getEquity();
            double marginLevel = m_s.getMarginLevel();   // 0 when no margin is used
            double floating    = m_s.getTotalProfit();
            int    open        = m_s.countOpenPositions();
            double ddPct = m_startBalance > 0
                         ? (m_startBalance - equity) / m_startBalance * 100.0 : 0.0;

            bool breach = (ddPct > m_cfg.maxDrawdownPct) ||
                          (open > 0 && marginLevel > 0 && marginLevel < m_cfg.minMarginLevel);
            if (breach) {
                m_s.closeAllPositions();              // kill-switch
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(m_cfg.intervalMs));
        }
    }
private:
    MT5Sugar& m_s; Config m_cfg; double m_startBalance{0};
};
```

**Key Sugar calls:** `getBalance` (baseline) · `getEquity` · `getMarginLevel` · `getTotalProfit` · `countOpenPositions` · `closeAllPositions`.

> **Why `marginLevel > 0` in the check:** a flat account reports `0%` margin level — without that guard the kill-switch would fire the moment you have no positions.

---

## 🔗 Usage Examples

### 1) Run it with defaults

```cpp
MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "password", "MetaQuotes-Demo");

RiskManager rm(sugar, RiskManager::Config{});
rm.run();   // monitors 5 cycles; flattens on a breach
```

### 2) Stricter limits, longer watch

```cpp
RiskManager::Config cfg;
cfg.maxDrawdownPct = 2.0;     // very tight drawdown budget
cfg.minMarginLevel = 300.0;   // demand a comfortable cushion
cfg.cycles         = 60;      // watch for ~48 s at 800 ms
RiskManager(sugar, cfg).run();
```

### 3) Run the monitor on a background thread while you trade

```cpp
#include <thread>
#include <atomic>

std::atomic<bool> stop{false};
std::thread guard([&]{
    double start = sugar.getBalance();
    while (!stop) {
        double dd = (start - sugar.getEquity()) / start * 100.0;
        bool open = sugar.countOpenPositions() > 0;
        double lvl = sugar.getMarginLevel();
        if (dd > 5.0 || (open && lvl > 0 && lvl < 150.0)) {
            sugar.closeAllPositions();
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
});

// ... place and manage trades on the main thread ...

stop = true; guard.join();
```

### 4) Daily-loss kill-switch (realized + floating)

Combine floating P/L with today's realized loss for a hard daily cap:

```cpp
double realizedToday = sugar.getProfitToday();     // closed trades
double floating      = sugar.getTotalProfit();     // open trades
if (realizedToday + floating < -200.0) {           // $200 daily stop
    sugar.closeAllPositions();
    std::puts("daily loss limit hit — flat for the day");
}
```

### 5) Report-only (no kill-switch)

Strip the action to get a pure health monitor:

```cpp
void report(MT5Sugar& sugar) {
    std::printf("equity %.2f | marginLevel %.1f%% | floatP/L %.2f | open %d\n",
                sugar.getEquity(), sugar.getMarginLevel(),
                sugar.getTotalProfit(), sugar.countOpenPositions());
}
```

---

## ⚠️ Pitfalls

* **Flat account = 0% margin level.** Always guard the margin-level check with `open > 0 && marginLevel > 0` (as the code does) or it misfires.
* **Baseline is the start balance**, not equity — drawdown is measured from where you began the run.
* **Kill-switch is all-or-nothing.** `closeAllPositions()` flattens *everything*; scope it (`closeAllBySymbol`) if you only manage one market.
* **Polling cadence.** A 1 s loop reacts within a second; shorten `intervalMs` for faster protection at the cost of more calls.

---

## 📚 Related

* [Strategies Overview](../Strategies.Master.Overview.md) · [Trailing Stop](./TrailingStop.md) · [Grid Trader](./GridTrader.md)
* Preset that dispatches to it: [Adaptive Market Mode](../Presets/AdaptiveMarketModePreset.md) (VOLATILE → Risk Manager)
* Sugar: [Balance / Margin](../../MT5Sugar/2.%20Balance_Margin.md) · [Risk Management](../../MT5Sugar/10.%20Risk_Management.md) · [Position Management](../../MT5Sugar/6.%20Position_Management.md)
