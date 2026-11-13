# Orchestrator · Trailing Stop

> Open one position, then **ratchet its stop loss up** as price advances — never down. The classic *"let winners run, cap the downside"* manager, built entirely on [`MT5Sugar`](../../MT5Sugar/MT5Sugar.API_Overview.md).

**Source:** `examples/demos/orchestrators/11_trailing_stop.cpp` · **Run:** `./build/demos 11` · **API level:** HIGH (Sugar)

!!! warning "Educational example"
    Places a small live `0.01` BUY on the **demo** account and closes it before returning. The heuristic (a fixed pip trail) is a template to build on, not a turnkey system.

---

## 🎯 What it does

1. Opens a market BUY.
2. Each cycle, reads the bid and computes a stop `trailPips` behind it.
3. Raises the SL **only if** the new level is higher than the current one (monotonic ratchet).
4. Closes the position at the end of the run.

## 🧩 The pattern

```
TrailingStop{config} → buyMarket → run(cycles):
    read bid → wantSL = bid - trailPips·pip → if wantSL > bestSL: raise SL → repeat
→ closePosition
```

## ⚙️ Config

```cpp
struct Config {
    double trailPips{20.0};   // keep the SL this many pips behind the best bid
    int    cycles{8};         // how many times to re-evaluate
    int    intervalMs{700};   // pause between cycles
};
```

---

## 🏗️ The orchestrator (annotated)

```cpp
class TrailingStop {
public:
    struct Config { double trailPips{20.0}; int cycles{8}; int intervalMs{700}; };

    TrailingStop(MT5Sugar& sugar, std::string symbol, double volume, Config cfg)
        : m_s(sugar), m_sym(std::move(symbol)), m_vol(volume), m_cfg(cfg) {
        // pip = point, scaled ×10 on 3/5-digit symbols (so "pips" means what a trader means)
        m_pip = sugar.getSymbolInfo(m_sym).point;
        int d = sugar.getSymbolDigits(m_sym);
        if (d == 3 || d == 5) m_pip *= 10.0;
    }

    void run() {
        uint64_t ticket = m_s.buyMarket(m_sym, m_vol);
        if (ticket == 0) return;                 // rejected (market closed?) — nothing to trail

        double bestSL = 0;
        for (int c = 1; c <= m_cfg.cycles; ++c) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_cfg.intervalMs));

            bool found = false;
            Position p = m_s.getPositionByTicket((int64_t)ticket, found);
            if (!found) return;                  // closed elsewhere — stop managing it

            double bid    = m_s.getBid(m_sym);
            double wantSL = bid - m_cfg.trailPips * m_pip;
            if (wantSL > bestSL) {               // only ever tighten
                bestSL = wantSL;
                m_s.modifyPositionSL((int64_t)ticket, bestSL);
            }
        }
        m_s.closePosition((int64_t)ticket);
    }
private:
    MT5Sugar& m_s; std::string m_sym; double m_vol, m_pip{0}; Config m_cfg;
};
```

**Key Sugar calls:** `buyMarket` · `getSymbolInfo` / `getSymbolDigits` (pip sizing) · `getBid` · `getPositionByTicket` · `modifyPositionSL` · `closePosition`.

---

## 🔗 Usage Examples

### 1) Run it with defaults

```cpp
MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "password", "MetaQuotes-Demo");

TrailingStop ts(sugar, "EURUSD", 0.01, TrailingStop::Config{});
ts.run();   // opens, trails for 8 cycles, closes
```

### 2) A tighter, faster trail

```cpp
TrailingStop::Config cfg;
cfg.trailPips  = 10;    // hug price closer
cfg.cycles     = 30;    // manage longer
cfg.intervalMs = 400;   // check more often
TrailingStop(sugar, "EURUSD", 0.02, cfg).run();
```

### 3) Trail an **existing** position instead of opening one

The demo opens its own trade; in a real strategy you often already hold one. The core is just the ratchet — apply it to any ticket:

```cpp
void trailExisting(MT5Sugar& sugar, int64_t ticket, double trailPips, int cycles) {
    bool found = false;
    Position p0 = sugar.getPositionByTicket(ticket, found);
    if (!found) return;
    double pip = sugar.getSymbolInfo(p0.symbol).point;
    if (sugar.getSymbolDigits(p0.symbol) % 2 == 1) pip *= 10.0;   // 3/5-digit → ×10

    double bestSL = p0.stopLoss;
    for (int c = 0; c < cycles; ++c) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        bool ok = false; Position p = sugar.getPositionByTicket(ticket, ok);
        if (!ok) return;
        double wantSL = sugar.getBid(p.symbol) - trailPips * pip;
        if (wantSL > bestSL) { bestSL = wantSL; sugar.modifyPositionSL(ticket, bestSL); }
    }
}
```

### 4) Break-even first, then trail

Lock the stop at entry once in profit, then start trailing:

```cpp
bool found = false;
Position p = sugar.getPositionByTicket(ticket, found);
double pip = sugar.getSymbolInfo(p.symbol).point * 10.0;   // 5-digit FX

if (found && sugar.getBid(p.symbol) > p.priceOpen + 10 * pip) {
    sugar.modifyPositionSL(ticket, p.priceOpen);            // break-even
    // ... then run the trail loop from example 3 ...
}
```

### 5) Respect the broker's minimum stop distance

A stop too close to price is rejected. Clamp the trail to `getMinStopLevel`:

```cpp
double pip      = sugar.getSymbolInfo("EURUSD").point * 10.0;
double minDist  = sugar.getMinStopLevel("EURUSD") * sugar.getSymbolInfo("EURUSD").point;
double trail    = std::max(20 * pip, minDist);             // never closer than the broker allows
double wantSL   = sugar.getBid("EURUSD") - trail;
```

---

## ⚠️ Pitfalls

* **Only raises the SL.** By design it never loosens the stop — a reversal just hits the last locked level.
* **BUY-only demo.** The example trails a long; for shorts, mirror it (`getAsk`, `wantSL = ask + trail`, only ever *lower* the SL).
* **Min stop distance.** Too tight a `trailPips` → the modify is rejected (see example 5).
* **Position may vanish.** `getPositionByTicket(found=false)` means it was closed elsewhere (TP hit, manual close) — the loop bails out cleanly.

---

## 📚 Related

* [Strategies Overview](../Strategies.Master.Overview.md) · [Grid Trader](./GridTrader.md) · [Risk Manager](./RiskManager.md)
* Preset that dispatches to it: [Adaptive Market Mode](../Presets/AdaptiveMarketModePreset.md) (TRENDING → Trailing Stop)
* Sugar: [Position Management](../../MT5Sugar/6.%20Position_Management.md) · [Prices / Quotes](../../MT5Sugar/3.%20Prices_Quotes.md)
