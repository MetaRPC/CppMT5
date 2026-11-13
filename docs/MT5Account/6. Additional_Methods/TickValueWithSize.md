# ✅ Getting Tick Value & Size (for P&L)

> **Request:** tick value, tick size and contract size for one or more symbols (`TickValueWithSizeData`) — the constants that turn points into money.

**API Information:**

* **Low-level API:** `MT5Account::tickValueWithSize(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `TickValueWithSize` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `TickValueWithSize(TickValueWithSizeRequest) → TickValueWithSizeReply`

---

## 💬 Just the essentials

* **What it is.** Returns tick value / tick size / contract size per symbol in a single batched call.
* **Why you need it.** Position sizing and P&L math: money per point, risk per lot.
* **Sanity check.** `tradetickvalue()` for a USD-quoted FX pair ≈ `$1` per point per lot (varies by symbol).

---

## 🧱 Signature

```cpp
// Returns tick value/size info for the requested symbols.
// Throws ApiError / TransportError on failure.
pb::TickValueWithSizeData tickValueWithSize(const pb::TickValueWithSizeRequest& req);
```

**Request / Reply:**

```protobuf
TickValueWithSizeRequest { repeated string symbol_names = 1; }

TickValueWithSizeReply {
  oneof response {
    TickValueWithSizeData data  = 1;   // { repeated TickSizeSymbol symbol_tick_size_infos = 1; }
    Error                 error = 2;
  }
}
```

---

## 🔽 Input

| Field          | C++ setter / type                       | Description                        |
| -------------- | --------------------------------------- | ---------------------------------- |
| `symbol_names` | `add_symbol_names("EURUSD")` (repeated) | One or more symbols to query.      |

```cpp
pb::TickValueWithSizeRequest r;
r.add_symbol_names("EURUSD");
r.add_symbol_names("GBPUSD");
```

---

## ⬆️ Output — `TickValueWithSizeData`

| Field                 | C++ accessor               | Element type    | Description            |
| --------------------- | -------------------------- | --------------- | ---------------------- |
| `SymbolTickSizeInfos` | `symbol_tick_size_infos()` | `TickSizeSymbol` | One record per symbol. |

### `TickSizeSymbol` — fields

| Accessor                  | Type     | Description                                   |
| ------------------------- | -------- | --------------------------------------------- |
| `index()`                 | `int32`  | Index of the symbol in the request.           |
| `name()`                  | `string` | Symbol name.                                  |
| `tradecontractsize()`     | `double` | Contract size (units per lot).                |
| `tradeticksize()`         | `double` | Minimal price change (tick size).             |
| `tradetickvalue()`        | `double` | Value of one tick in the account currency.    |
| `tradetickvalueprofit()`  | `double` | Tick value for a profitable move.             |
| `tradetickvalueloss()`    | `double` | Tick value for a losing move.                 |

---

## 🧩 Notes & Tips

* **Money per point** ≈ `tradetickvalue() × (SYMBOL_POINT / tradeticksize())`. For many FX symbols `point == tick size`, so it's just `tradetickvalue()` per point per lot.
* **Batch friendly.** Pass your whole watchlist at once instead of one call per symbol.
* **Profit vs loss tick value** can differ for some instruments — use the matching one for precise risk math.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::TickValueWithSizeRequest r;
r.add_symbol_names("EURUSD");
r.add_symbol_names("XAUUSD");

for (const auto& s : account.tickValueWithSize(r).symbol_tick_size_infos())
    std::printf("%-8s tickValue=%.5f tickSize=%.5f contract=%.1f\n",
                s.name().c_str(), s.tradetickvalue(), s.tradeticksize(), s.tradecontractsize());
```

### Risk-based lot sizing

```cpp
// lots so that `stopPoints` of adverse move ≈ `riskMoney`
double lotsForRisk(const pb::TickSizeSymbol& s, double stopPoints, double riskMoney) {
    double perPointPerLot = s.tradetickvalue();     // assuming point == tick size
    return riskMoney / (stopPoints * perPointPerLot);
}
```

---

## 📚 See Also

* [SymbolInfoDouble](../2.%20Symbol_Information/SymbolInfoDouble.md) — single tick-value/point reads
* [OrderCalcProfit](../4.%20Trading_Operations/OrderCalcProfit.md) — server-side profit for a move
* **Friendly layer:** `MT5Service::tickValueWithSize({"EURUSD","GBPUSD"})` returns named structs.
