# ✅ Getting OHLC Bars (Price History)

> **Request:** historical OHLC bars for a symbol and timeframe (`PriceHistoryData`).

**API Information:**

* **Low-level API:** `MT5Account::priceHistory(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `PriceHistory` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `PriceHistory(PriceHistoryRequest) → PriceHistoryReply`

---

## 💬 Just the essentials

* **What it is.** Returns a list of OHLC bars (candles) for a symbol/timeframe in a time window.
* **Why you need it.** Indicators, backtests, charts — anything that needs candles.
* **Sanity check.** `bars_size() > 0` for an active symbol and a sane window.

---

## 🎯 Purpose

* Pull recent candles (e.g. last 200 × H1) to seed an indicator.
* Export a date range of bars for analysis.
* Compute volatility/ATR, highs/lows, moving averages.

---

## 🧱 Signature

```cpp
// Returns OHLC bars for req.symbol() at req.timeframe().
// Throws ApiError / TransportError on failure.
pb::PriceHistoryData priceHistory(const pb::PriceHistoryRequest& req);
```

**Request / Reply:**

```protobuf
PriceHistoryRequest {
  string symbol        = 1;
  int32  timeframe     = 2;   // MT5 ENUM_TIMEFRAMES code
  int64  from_time     = 3;   // epoch seconds, broker tz
  int64  to_time       = 4;
  int32  max_bars      = 5;
  int64  from_time_utc = 6;   // epoch seconds, UTC
  int64  to_time_utc   = 7;
}

PriceHistoryReply {
  oneof response {
    PriceHistoryData data  = 1;   // { repeated PriceBar bars = 1; }
    Error            error = 2;
  }
}
```

---

## 🔽 Input

| Field           | C++ setter / type                  | Description                                               |
| --------------- | ---------------------------------- | -------------------------------------------------------- |
| `symbol`        | `set_symbol("EURUSD")`             | Instrument.                                              |
| `timeframe`     | `set_timeframe(16385)`             | MT5 `ENUM_TIMEFRAMES` code (M1=1, M5=5, M15=15, H1=16385, D1=16408, …). See [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md). |
| `from_time_utc` | `set_from_time_utc(epoch)`         | Window start (UTC epoch seconds). Prefer the `*_utc` fields. |
| `to_time_utc`   | `set_to_time_utc(epoch)`           | Window end (UTC epoch seconds).                          |
| `max_bars`      | `set_max_bars(500)`                | Cap on the number of bars returned.                      |

---

## ⬆️ Output — `PriceHistoryData`

| Field  | C++ accessor | Element type | Description          |
| ------ | ------------ | ------------ | -------------------- |
| `Bars` | `bars()`     | `PriceBar`   | OHLC bars.           |

Use `bars_size()`, `bars(i)`, or range-for.

### `PriceBar` — fields

| Accessor        | Type      | Description                                   |
| --------------- | --------- | --------------------------------------------- |
| `time()`        | `int64`   | Bar open time (epoch seconds, broker tz).     |
| `time_utc()`    | `int64`   | Bar open time (epoch seconds, UTC).           |
| `open()`        | `double`  | Open price.                                   |
| `high()`        | `double`  | High price.                                   |
| `low()`         | `double`  | Low price.                                    |
| `close()`       | `double`  | Close price.                                  |
| `tick_volume()` | `int64`   | Tick volume.                                  |
| `real_volume()` | `int64`   | Real (exchange) volume.                       |
| `spread()`      | `int32`   | Spread (points) at the bar.                   |

---

## 🧩 Notes & Tips

* **Use UTC fields.** `from_time_utc` / `to_time_utc` avoid broker-timezone ambiguity; each bar also carries `time_utc()`.
* **Bound the result.** Set `max_bars` so a wide window doesn't return an enormous payload.
* **Large histories.** `ClientOptions::maxReceiveMessageBytes` (default 64 MB) covers big pulls.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Last 200 H1 bars

```cpp
#include <ctime>
std::time_t now = std::time(nullptr);

pb::PriceHistoryRequest r;
r.set_symbol("EURUSD");
r.set_timeframe(16385);                       // H1
r.set_from_time_utc(now - 200 * 3600);
r.set_to_time_utc(now);
r.set_max_bars(200);

pb::PriceHistoryData d = account.priceHistory(r);
std::printf("Got %d bars\n", d.bars_size());
for (const auto& b : d.bars())
    std::printf("O %.5f H %.5f L %.5f C %.5f\n", b.open(), b.high(), b.low(), b.close());
```

### 2) Highest high / lowest low over the window

```cpp
double hi = -1e18, lo = 1e18;
for (const auto& b : account.priceHistory(r).bars()) {
    hi = std::max(hi, b.high());
    lo = std::min(lo, b.low());
}
std::printf("Range: %.5f .. %.5f\n", lo, hi);
```

---

## 📚 See Also

* [SymbolInfoTick](../2.%20Symbol_Information/SymbolInfoTick.md) — the current tick (not bars)
* [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md) — timeframe codes
* **Friendly layer:** `MT5Service::lastBars("EURUSD", Timeframe::H1, 200)` returns named `Bar` structs with UTC times.
