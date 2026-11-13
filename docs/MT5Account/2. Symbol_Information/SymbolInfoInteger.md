# ✅ Getting a Single Integer Symbol Property

> **Request:** one integer symbol property (`SymbolInfoIntegerData`) from **MT5** — digits, spread, trade mode, session times, flags — selected by a property enum.

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoInteger(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoInteger` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoInteger(SymbolInfoIntegerRequest) → SymbolInfoIntegerReply`

---

## 💬 Just the essentials

* **What it is.** One RPC that returns a **single `int64`** property of a symbol, chosen by a `SymbolInfoIntegerProperty` enum.
* **Why you need it.** `SYMBOL_DIGITS` and `SYMBOL_SPREAD` for formatting; `SYMBOL_TRADE_MODE` to know whether trading is allowed; time fields for sessions.
* **Sanity check.** `SYMBOL_DIGITS` for EURUSD is usually `5`.

---

## 🎯 Purpose

* Read `SYMBOL_DIGITS` to round prices and `SYMBOL_SPREAD` (in points).
* Check `SYMBOL_TRADE_MODE` before trying to trade a symbol.
* Read `SYMBOL_TIME` (last quote time) for staleness checks.

---

## 🧱 Signature

```cpp
// Returns the single int64 property selected by req.type() for req.symbol().
// Throws ApiError / TransportError on failure.
pb::SymbolInfoIntegerData symbolInfoInteger(const pb::SymbolInfoIntegerRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoIntegerRequest {
  string                    symbol = 1;
  SymbolInfoIntegerProperty type   = 2;
}

SymbolInfoIntegerReply {
  oneof response {
    SymbolInfoIntegerData data  = 1;   // { int64 value = 1; int64 value_utc = 2; }
    Error                 error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type                              | Description                    |
| -------- | ---------------------------------------------- | ------------------------------ |
| `symbol` | `set_symbol("EURUSD")` — string                | The symbol.                    |
| `type`   | `set_type(pb::SYMBOL_DIGITS)` — `SymbolInfoIntegerProperty` | Which integer property to read. |

---

## ⬆️ Output — `SymbolInfoIntegerData`

| Field      | C++ accessor  | C++ type  | Description                                                        |
| ---------- | ------------- | --------- | ----------------------------------------------------------------- |
| `Value`    | `value()`     | `int64_t` | The value of the chosen property (broker time zone for time-based ones). |
| `ValueUtc` | `value_utc()` | `int64_t` | For time-based properties (`SYMBOL_TIME`, `SYMBOL_START_TIME`, …), the same moment in UTC. `0` for non-time properties. |

---

## 🧱 Property enum — `SymbolInfoIntegerProperty` (common values)

| Constant                  | Value | Meaning                                       |
| ------------------------- | ----- | --------------------------------------------- |
| `SYMBOL_EXIST`            | 6     | Symbol exists (0/1).                          |
| `SYMBOL_SELECT`           | 7     | Symbol is selected in Market Watch (0/1).     |
| `SYMBOL_VISIBLE`          | 8     | Symbol is visible in Market Watch (0/1).      |
| `SYMBOL_TIME`             | 15    | Time of the last quote (epoch seconds).       |
| `SYMBOL_DIGITS`           | 17    | Decimal digits in the price.                  |
| `SYMBOL_SPREAD_FLOAT`     | 18    | Spread is floating (0/1).                     |
| `SYMBOL_SPREAD`           | 19    | Current spread in points.                     |
| `SYMBOL_TRADE_CALC_MODE`  | 21    | Contract price calculation mode.              |
| `SYMBOL_TRADE_MODE`       | 22    | Order execution permissions for the symbol.   |
| `SYMBOL_START_TIME`       | 23    | Trading start date (epoch seconds; 0 if none).|
| `SYMBOL_EXPIRATION_TIME`  | 24    | Trading end date (epoch seconds; 0 if none).  |
| `SYMBOL_TRADE_STOPS_LEVEL`| 25    | Min distance (points) for SL/TP from price.   |
| `SYMBOL_TRADE_FREEZE_LEVEL`| 26   | Freeze distance (points) for orders.          |
| `SYMBOL_FILLING_MODE`     | 32    | Allowed order filling modes (bit flags).      |

---

## 🧩 Notes & Tips

* **Time fields:** for `SYMBOL_TIME` / `SYMBOL_START_TIME` / `SYMBOL_EXPIRATION_TIME` use `value_utc()` to avoid broker-timezone confusion.
* **Flags are int.** Properties like `SYMBOL_SELECT` / `SYMBOL_VISIBLE` return `0`/`1`.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Digits & spread

```cpp
int64_t si(MT5Account& a, const std::string& s, pb::SymbolInfoIntegerProperty p) {
    pb::SymbolInfoIntegerRequest r; r.set_symbol(s); r.set_type(p);
    return a.symbolInfoInteger(r).value();
}

std::printf("EURUSD digits=%lld spread=%lld points\n",
            (long long)si(account, "EURUSD", pb::SYMBOL_DIGITS),
            (long long)si(account, "EURUSD", pb::SYMBOL_SPREAD));
```

### 2) Is the symbol tradable right now?

```cpp
int64_t mode = si(account, "EURUSD", pb::SYMBOL_TRADE_MODE);
// 0 = disabled; non-zero = some form of trading allowed (long/short/close-only…)
std::printf("Trade mode: %lld\n", (long long)mode);
```

### 3) Last quote time (UTC) for a staleness check

```cpp
#include <ctime>
pb::SymbolInfoIntegerRequest r; r.set_symbol("EURUSD"); r.set_type(pb::SYMBOL_TIME);
pb::SymbolInfoIntegerData d = account.symbolInfoInteger(r);
std::time_t utc = (std::time_t)d.value_utc();
char buf[32]; std::strftime(buf, sizeof buf, "%F %T", std::gmtime(&utc));
std::printf("Last quote (UTC): %s\n", buf);
```

---

## 📚 See Also

* [SymbolInfoDouble](./SymbolInfoDouble.md) — point, tick value, volume limits
* [SymbolInfoString](./SymbolInfoString.md) — description, base/profit currency
* [SymbolInfoTick](./SymbolInfoTick.md) — the full current tick in one call
* **Friendly layer:** `MT5Service::symbolParams("EURUSD")` returns `digits`, `spread`, `tradeMode`, … already named.
