# MT5Account · Additional Methods — Overview

> Advanced symbol data, trading sessions, margin rates, batch symbol parameters, tick value/size, and bar history — the methods that don't fit the other groups.

## 📁 What lives here

* **[SymbolInfoMarginRate](./SymbolInfoMarginRate.md)** — initial/maintenance margin **rate** multipliers for a symbol + order type. (`MarketInfo`)
* **[SymbolInfoSessionQuote](./SymbolInfoSessionQuote.md)** — **quote** session [from, to] per weekday. (`MarketInfo`)
* **[SymbolInfoSessionTrade](./SymbolInfoSessionTrade.md)** — **trade** session [from, to] per weekday. (`MarketInfo`)
* **[SymbolParamsMany](./SymbolParamsMany.md)** — full `SymbolParameters` (100+ fields) for many symbols, paginated. (`AccountHelper`)
* **[TickValueWithSize](./TickValueWithSize.md)** — tick value / tick size / contract size for P&L, batched. (`AccountHelper`)
* **[PriceHistory](./PriceHistory.md)** — OHLC bars for a symbol/timeframe. (`MarketInfo`)

---

## 🧭 Plain English

* "**How much margin does this symbol really need?**" → `symbolInfoMarginRate()` (+ [OrderCalcMargin](../4.%20Trading_Operations/OrderCalcMargin.md)).
* "**When is it quoting / tradable?**" → `symbolInfoSessionQuote()` / `symbolInfoSessionTrade()`.
* "**Give me everything about my whole watchlist.**" → `symbolParamsMany()`.
* "**What's a point worth in money?**" → `tickValueWithSize()`.
* "**I need candles.**" → `priceHistory()`.

---

## Quick choose

| If you need…                         | Use                      | Returns                         |
| ------------------------------------ | ------------------------ | ------------------------------- |
| Margin rate multipliers              | `symbolInfoMarginRate`   | `initial_margin_rate`, `maintenance_margin_rate` |
| Quote session window                 | `symbolInfoSessionQuote` | `from`, `to` (Timestamp)        |
| Trade session window                 | `symbolInfoSessionTrade` | `from`, `to` (Timestamp)        |
| Full specs for many symbols          | `symbolParamsMany`       | `SymbolParameters[]` + pagination |
| Tick value/size for P&L              | `tickValueWithSize`      | `TickSizeSymbol[]`              |
| OHLC bars                            | `priceHistory`           | `PriceBar[]`                    |

---

## ❌ Cross-refs & gotchas

* **Two order-type enums.** `symbolInfoMarginRate` uses `ENUM_ORDER_TYPE` (`ORDER_TYPE_BUY`…), while `orderSend` uses `TMT5_ENUM_ORDER_TYPE` (`TMT5_ORDER_TYPE_BUY`…) and `orderCalc*` use `ENUM_ORDER_TYPE_TF`. Same values, different names.
* **Sessions repeat.** A weekday can have several sessions — walk `session_index` from 0.
* **Batch beats loops.** `symbolParamsMany` / `tickValueWithSize` fetch many symbols at once — prefer them over per-symbol calls.
* **Timeframe is an int code.** `priceHistory`'s `timeframe` is the raw MT5 `ENUM_TIMEFRAMES` value (H1 = 16385). See [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md).
* **Use UTC fields.** `priceHistory` and the session data expose UTC variants — prefer them.

---

## 🟢 Minimal snippets

```cpp
// Margin rate
pb::SymbolInfoMarginRateRequest mr; mr.set_symbol("EURUSD"); mr.set_order_type(pb::ORDER_TYPE_BUY);
auto rate = account.symbolInfoMarginRate(mr);
std::printf("initial %.3f\n", rate.initial_margin_rate());
```

```cpp
// Tick value
pb::TickValueWithSizeRequest tv; tv.add_symbol_names("EURUSD");
for (const auto& s : account.tickValueWithSize(tv).symbol_tick_size_infos())
    std::printf("%s tickValue %.5f\n", s.name().c_str(), s.tradetickvalue());
```

```cpp
// 100 M15 bars
#include <ctime>
std::time_t now = std::time(nullptr);
pb::PriceHistoryRequest ph;
ph.set_symbol("EURUSD"); ph.set_timeframe(15);
ph.set_from_time_utc(now - 100*15*60); ph.set_to_time_utc(now); ph.set_max_bars(100);
std::printf("%d bars\n", account.priceHistory(ph).bars_size());
```

---

## See also

* **Symbols:** [Symbol Information overview](../2.%20Symbol_Information/Symbol_Information.Overview.md)
* **Trading:** [OrderCalcMargin](../4.%20Trading_Operations/OrderCalcMargin.md) — turn margin rates into amounts
* **Enums:** [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md) — timeframe & order-type codes
