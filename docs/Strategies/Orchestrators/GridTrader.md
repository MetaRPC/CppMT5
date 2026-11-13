# Orchestrator · Grid Trader

> Lay a **symmetric grid of pending orders** around the current price — buy-limits below, sell-limits above — then list and remove them. Demonstrates pending-order placement + cleanup on [`MT5Sugar`](../../MT5Sugar/MT5Sugar.API_Overview.md) (+ `service()`).

**Source:** `examples/demos/orchestrators/13_grid_trader.cpp` · **Run:** `./build/demos 13` · **API level:** HIGH (Sugar) + `service()`

!!! warning "Educational example"
    Places small live **pending** orders on the **demo** account and deletes them all before returning — nothing is left open. The fixed grid is a template, not a strategy.

---

## 🎯 What it does

1. Reads bid/ask.
2. For each level `i = 1…levels`, places a **buy-limit** `i·stepPips` below and a **sell-limit** `i·stepPips` above.
3. Lists the live pending orders via `service().openedOrders().orders`.
4. Deletes every ticket it placed (`closePosition` deletes a pending by ticket).

## 🧩 The pattern

```
GridTrader{config} → for i in 1..levels: buyLimit(bid - i·step) + sellLimit(ask + i·step)
→ list (service().openedOrders) → remove (closePosition per ticket)
```

## ⚙️ Config

```cpp
struct Config {
    int    levels{2};        // pending orders on each side
    double stepPips{20.0};   // distance between levels
    double volume{0.01};
};
```

---

## 🏗️ The orchestrator (annotated)

```cpp
class GridTrader {
public:
    struct Config { int levels{2}; double stepPips{20.0}; double volume{0.01}; };

    GridTrader(MT5Sugar& sugar, std::string symbol, Config cfg)
        : m_s(sugar), m_sym(std::move(symbol)), m_cfg(cfg) {
        m_pip = sugar.getSymbolInfo(m_sym).point;
        int d = sugar.getSymbolDigits(m_sym);
        if (d == 3 || d == 5) m_pip *= 10.0;
    }

    void run() {
        double bid = m_s.getBid(m_sym), ask = m_s.getAsk(m_sym);

        std::vector<uint64_t> tickets;
        for (int i = 1; i <= m_cfg.levels; ++i) {
            double below = bid - i * m_cfg.stepPips * m_pip;   // buy-limit under price
            double above = ask + i * m_cfg.stepPips * m_pip;   // sell-limit over price
            uint64_t b = m_s.buyLimit (m_sym, m_cfg.volume, below);
            uint64_t s = m_s.sellLimit(m_sym, m_cfg.volume, above);
            if (b) tickets.push_back(b);
            if (s) tickets.push_back(s);
        }
        if (tickets.empty()) return;              // market closed / too close to price

        auto pendings = m_s.service().openedOrders().orders;   // the friendly PendingOrder list

        for (uint64_t t : tickets) m_s.closePosition((int64_t)t);   // clean up everything we placed
    }
private:
    MT5Sugar& m_s; std::string m_sym; double m_pip{0}; Config m_cfg;
};
```

**Key Sugar/Service calls:** `getBid` / `getAsk` · `buyLimit` / `sellLimit` · `service().openedOrders()` · `closePosition` (deletes a pending).

---

## 🔗 Usage Examples

### 1) Run it with defaults

```cpp
MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "password", "MetaQuotes-Demo");

GridTrader gt(sugar, "EURUSD", GridTrader::Config{});
gt.run();   // 2 levels/side, 20-pip step, places → lists → removes
```

### 2) A wider, deeper grid

```cpp
GridTrader::Config cfg;
cfg.levels   = 5;       // 5 orders each side (10 pending total)
cfg.stepPips = 15;      // tighter spacing
cfg.volume   = 0.02;
GridTrader(sugar, "EURUSD", cfg).run();
```

### 3) Place a grid and **keep** it (manage elsewhere)

Drop the cleanup loop and return the tickets so another component can manage fills:

```cpp
std::vector<uint64_t> placeGrid(MT5Sugar& sugar, const std::string& sym,
                                int levels, double stepPips, double vol) {
    double pip = sugar.getSymbolInfo(sym).point;
    if (sugar.getSymbolDigits(sym) % 2 == 1) pip *= 10.0;
    double bid = sugar.getBid(sym), ask = sugar.getAsk(sym);

    std::vector<uint64_t> t;
    for (int i = 1; i <= levels; ++i) {
        if (uint64_t b = sugar.buyLimit (sym, vol, bid - i*stepPips*pip)) t.push_back(b);
        if (uint64_t s = sugar.sellLimit(sym, vol, ask + i*stepPips*pip)) t.push_back(s);
    }
    return t;   // caller decides when to cancel
}
```

### 4) One-sided grid (buy-the-dip ladder only)

```cpp
double pip = sugar.getSymbolInfo("EURUSD").point * 10.0;
double bid = sugar.getBid("EURUSD");
for (int i = 1; i <= 4; ++i)
    sugar.buyLimit("EURUSD", 0.01, bid - i * 15 * pip);   // ladder of buy-limits below
```

### 5) Cancel only the far orders, keep the near ones

```cpp
auto placed = placeGrid(sugar, "EURUSD", 5, 15, 0.01);   // from example 3
// ... later, thin the grid: cancel every other ticket ...
for (size_t i = 0; i < placed.size(); ++i)
    if (i % 2 == 0) sugar.closePosition((int64_t)placed[i]);
```

---

## ⚠️ Pitfalls

* **No fill management.** The demo never lets the grid fill — it places, lists, removes. A real grid needs rules for filled legs (SL/TP, rebalancing).
* **Too close to price is rejected.** Limits inside the spread / min-stop distance won't be accepted — `stepPips` must clear it.
* **`closePosition` on a ticket** deletes a *pending order* and closes a *position* — same call, context decides. Track which tickets are which.
* **Symbol must be selectable/open.** An empty `tickets` means the market was closed or prices too tight.

---

## 📚 Related

* [Strategies Overview](../Strategies.Master.Overview.md) · [Trailing Stop](./TrailingStop.md) · [Risk Manager](./RiskManager.md)
* Preset that dispatches to it: [Adaptive Market Mode](../Presets/AdaptiveMarketModePreset.md) (RANGING → Grid Trader)
* Sugar: [Simple Trading](../../MT5Sugar/4.%20Simple_Trading.md) · Service: [Position & Orders](../../MT5Service/3.%20Position_Orders_Methods.md)
