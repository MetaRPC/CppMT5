# ✅ Getting the Current Tick (Quote)

> **Request:** the latest quote for a symbol (`MrpcMqlTick`) from **MT5** — bid, ask, last, volume and timestamps in one call.

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoTick(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoTick` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoTick(SymbolInfoTickRequest) → SymbolInfoTickRequestReply`

---

## 💬 Just the essentials

* **What it is.** A single RPC returning the most recent tick for one symbol.
* **Why you need it.** Current price before placing a trade; spread; a lightweight price feed via polling.
* **Sanity check.** `bid()` and `ask()` are > 0 and `ask() >= bid()`.

> **Note.** `symbolInfoTick()` returns the tick payload **directly** (`pb::MrpcMqlTick`), not a `*Data` wrapper.

---

## 🎯 Purpose

* Read the live bid/ask to decide entry price and compute spread (`ask - bid`).
* Timestamp a quote (`time()` broker tz, `time_utc()` UTC; `time_msc*` in milliseconds).
* Build a simple price poller (for push updates use [OnSymbolTick](../7.%20Streaming_Methods/OnSymbolTick.md) instead).

---

## 🧱 Signature

```cpp
// Returns the latest tick for req.symbol(). Throws ApiError / TransportError.
pb::MrpcMqlTick symbolInfoTick(const pb::SymbolInfoTickRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoTickRequest { string symbol = 1; }

SymbolInfoTickRequestReply {
  oneof response {
    MrpcMqlTick data  = 1;
    Error       error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type              | Description                         |
| -------- | ------------------------------ | ----------------------------------- |
| `symbol` | `set_symbol("EURUSD")` — string | The symbol to quote. Must be in Market Watch (see [SymbolSelect](./SymbolSelect.md)). |

```cpp
pb::SymbolInfoTickRequest r;
r.set_symbol("EURUSD");
pb::MrpcMqlTick tick = account.symbolInfoTick(r);
```

---

## ⬆️ Output — `MrpcMqlTick`

| Field          | C++ accessor     | C++ type    | Description                                            |
| -------------- | ---------------- | ----------- | ------------------------------------------------------ |
| `Bid`          | `bid()`          | `double`    | Best bid (price to sell at).                           |
| `Ask`          | `ask()`          | `double`    | Best ask (price to buy at).                            |
| `Last`         | `last()`         | `double`    | Last deal price (for exchange instruments).            |
| `Volume`       | `volume()`       | `uint64_t`  | Volume for the last deal (integer).                    |
| `VolumeReal`   | `volume_real()`  | `double`    | Volume with greater precision.                         |
| `Time`         | `time()`         | `int64_t`   | Tick time (epoch seconds), broker server time zone.    |
| `TimeUtc`      | `time_utc()`     | `int64_t`   | Tick time (epoch seconds), UTC.                        |
| `TimeMsc`      | `time_msc()`     | `int64_t`   | Tick time in milliseconds, broker time zone.           |
| `TimeMscUtc`   | `time_msc_utc()` | `int64_t`   | Tick time in milliseconds, UTC.                        |
| `Flags`        | `flags()`        | `uint32_t`  | Tick change flags (bid/ask/last/volume changed, etc.). |

---

## 🧩 Notes & Tips

* **Symbol must be selected.** If a symbol is not in Market Watch, call [SymbolSelect](./SymbolSelect.md) first (or the quote may be stale/empty).
* **Spread** = `ask() - bid()`; in points, divide by the symbol's `SYMBOL_POINT` ([SymbolInfoDouble](./SymbolInfoDouble.md)).
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method (`ApiError` / `TransportError`, `ClientOptions::unaryTimeoutMs`).
* **Streaming alternative.** For continuous updates don't poll in a tight loop — subscribe with [OnSymbolTick](../7.%20Streaming_Methods/OnSymbolTick.md).

---

## 🔗 Usage Examples

### 1) Bid / ask / spread

```cpp
pb::SymbolInfoTickRequest r;
r.set_symbol("EURUSD");
pb::MrpcMqlTick t = account.symbolInfoTick(r);

std::printf("Bid %.5f  Ask %.5f  Spread %.5f\n",
            t.bid(), t.ask(), t.ask() - t.bid());
```

### 2) A helper that returns just the mid price

```cpp
double midPrice(MT5Account& a, const std::string& symbol) {
    pb::SymbolInfoTickRequest r;
    r.set_symbol(symbol);
    pb::MrpcMqlTick t = a.symbolInfoTick(r);
    return (t.bid() + t.ask()) / 2.0;
}
```

### 3) Timestamped quote (UTC)

```cpp
#include <ctime>

pb::SymbolInfoTickRequest r; r.set_symbol("XAUUSD");
pb::MrpcMqlTick t = account.symbolInfoTick(r);

std::time_t utc = (std::time_t)t.time_utc();
char buf[32]; std::strftime(buf, sizeof buf, "%F %T", std::gmtime(&utc));
std::printf("[%s UTC] XAUUSD bid=%.2f ask=%.2f\n", buf, t.bid(), t.ask());
```

### 4) Error handling

```cpp
try {
    pb::SymbolInfoTickRequest r; r.set_symbol("EURUSD");
    pb::MrpcMqlTick t = account.symbolInfoTick(r);
    std::printf("EURUSD %.5f / %.5f\n", t.bid(), t.ask());
} catch (const ApiError& e) {
    std::printf("Server error (symbol unknown?): %s\n", e.what());
} catch (const TransportError& e) {
    std::printf("Transport error: %s\n", e.what());
}
```

---

## 📚 See Also

* [SymbolInfoDouble](./SymbolInfoDouble.md) — point size, tick value, volume limits (needed to interpret the price)
* [SymbolSelect](./SymbolSelect.md) — make sure the symbol is in Market Watch first
* [OnSymbolTick](../7.%20Streaming_Methods/OnSymbolTick.md) — push-based live tick stream
* **Friendly layer:** `MT5Service::tick("EURUSD")` returns a named `Tick` struct with UTC time.
