# ✅ Getting a Single Double Symbol Property

> **Request:** one floating-point symbol property (`SymbolInfoDoubleData`) from **MT5** — bid, ask, point, tick value, volume limits, swaps, etc. — selected by a property enum.

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoDouble(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoDouble` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoDouble(SymbolInfoDoubleRequest) → SymbolInfoDoubleReply`

---

## 💬 Just the essentials

* **What it is.** One RPC that returns a **single `double`** property of a symbol, chosen by a `SymbolInfoDoubleProperty` enum.
* **Why you need it.** The constants that turn a raw price into money: `SYMBOL_POINT`, `SYMBOL_TRADE_TICK_VALUE`, `SYMBOL_TRADE_TICK_SIZE`, `SYMBOL_VOLUME_MIN/MAX/STEP`.
* **Sanity check.** `SYMBOL_BID` should match `symbolInfoTick().bid()`.

---

## 🎯 Purpose

* Read `SYMBOL_POINT` and `SYMBOL_DIGITS` to format/round prices correctly.
* Read `SYMBOL_VOLUME_MIN / MAX / STEP` to validate and normalize lot sizes.
* Read `SYMBOL_TRADE_TICK_VALUE` + `SYMBOL_TRADE_TICK_SIZE` for P/L and risk math.

---

## 🧱 Signature

```cpp
// Returns the single double property selected by req.type() for req.symbol().
// Throws ApiError / TransportError on failure.
pb::SymbolInfoDoubleData symbolInfoDouble(const pb::SymbolInfoDoubleRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoDoubleRequest {
  string                   symbol = 1;
  SymbolInfoDoubleProperty type   = 2;
}

SymbolInfoDoubleReply {
  oneof response {
    SymbolInfoDoubleData data  = 1;   // { double value = 1; }
    Error                error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type                              | Description                 |
| -------- | ---------------------------------------------- | --------------------------- |
| `symbol` | `set_symbol("EURUSD")` — string                | The symbol.                 |
| `type`   | `set_type(pb::SYMBOL_POINT)` — `SymbolInfoDoubleProperty` | Which double property to read. |

```cpp
pb::SymbolInfoDoubleRequest r;
r.set_symbol("EURUSD");
r.set_type(pb::SYMBOL_POINT);
double point = account.symbolInfoDouble(r).value();   // e.g. 0.00001
```

---

## ⬆️ Output — `SymbolInfoDoubleData`

| Field   | C++ accessor | C++ type | Description                     |
| ------- | ------------ | -------- | ------------------------------- |
| `Value` | `value()`    | `double` | The value of the chosen property. |

---

## 🧱 Property enum — `SymbolInfoDoubleProperty` (common values)

> 60 values total (`SYMBOL_BID … SYMBOL_COUNT`); the most used:

| Constant                        | Value | Meaning                                   |
| ------------------------------- | ----- | ----------------------------------------- |
| `SYMBOL_BID`                    | 0     | Current best bid.                         |
| `SYMBOL_ASK`                    | 3     | Current best ask.                         |
| `SYMBOL_LAST`                   | 6     | Last deal price.                          |
| `SYMBOL_POINT`                  | 13    | Point size (smallest price step).         |
| `SYMBOL_TRADE_TICK_VALUE`       | 14    | Value of one tick in the account currency.|
| `SYMBOL_TRADE_TICK_SIZE`        | 17    | Minimal price change (tick size).         |
| `SYMBOL_TRADE_CONTRACT_SIZE`    | 18    | Contract size (units per lot).            |
| `SYMBOL_VOLUME_MIN`             | 22    | Minimum volume (lots).                    |
| `SYMBOL_VOLUME_MAX`             | 23    | Maximum volume (lots).                    |
| `SYMBOL_VOLUME_STEP`            | 24    | Volume step (lot increment).              |
| `SYMBOL_VOLUME_LIMIT`           | 25    | Max total volume per symbol.              |
| `SYMBOL_SWAP_LONG`              | 26    | Swap charged on long positions.           |
| `SYMBOL_SWAP_SHORT`             | 27    | Swap charged on short positions.          |
| `SYMBOL_MARGIN_INITIAL`         | 35    | Initial margin per lot.                   |

---

## 🧩 Notes & Tips

* **Point vs tick size.** For most FX symbols `SYMBOL_POINT` equals `SYMBOL_TRADE_TICK_SIZE`, but not always — use the right one for your math.
* **Money per point** ≈ `SYMBOL_TRADE_TICK_VALUE × (SYMBOL_POINT / SYMBOL_TRADE_TICK_SIZE)`.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.
* **Batch alternative.** Need many fields for many symbols at once? Use [SymbolParamsMany](../6.%20Additional_Methods/SymbolParamsMany.md).

---

## 🔗 Usage Examples

### 1) Point and digits-driven formatting

```cpp
double sd(MT5Account& a, const std::string& s, pb::SymbolInfoDoubleProperty p) {
    pb::SymbolInfoDoubleRequest r; r.set_symbol(s); r.set_type(p);
    return a.symbolInfoDouble(r).value();
}

double point = sd(account, "EURUSD", pb::SYMBOL_POINT);
std::printf("Point size: %.5f\n", point);
```

### 2) Validate a lot size against min/max/step

```cpp
double vmin = sd(account, "EURUSD", pb::SYMBOL_VOLUME_MIN);
double vmax = sd(account, "EURUSD", pb::SYMBOL_VOLUME_MAX);
double step = sd(account, "EURUSD", pb::SYMBOL_VOLUME_STEP);

double wanted = 0.037;
double norm = std::round(wanted / step) * step;          // snap to step
norm = std::max(vmin, std::min(vmax, norm));             // clamp
std::printf("Normalized volume: %.2f (min %.2f / step %.2f)\n", norm, vmin, step);
```

### 3) Spread in points

```cpp
double bid = sd(account, "EURUSD", pb::SYMBOL_BID);
double ask = sd(account, "EURUSD", pb::SYMBOL_ASK);
double point = sd(account, "EURUSD", pb::SYMBOL_POINT);
std::printf("Spread: %.1f points\n", (ask - bid) / point);
```

---

## 📚 See Also

* [SymbolInfoInteger](./SymbolInfoInteger.md) — digits, spread, trade mode (integer properties)
* [SymbolInfoString](./SymbolInfoString.md) — description, base/profit currency
* [SymbolInfoTick](./SymbolInfoTick.md) — the full current tick in one call
* [SymbolParamsMany](../6.%20Additional_Methods/SymbolParamsMany.md) — many fields for many symbols at once
* **Friendly layer:** `MT5Service::symbolParams("EURUSD")` returns a named `SymbolInfo` with `point`, `digits`, `volumeMin`, …
