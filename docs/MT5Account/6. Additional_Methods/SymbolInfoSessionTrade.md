# ✅ Getting Trade Session Times

> **Request:** the tradable session window for a symbol on a given weekday (`SymbolInfoSessionTradeData`).

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoSessionTrade(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoSessionTrade` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoSessionTrade(SymbolInfoSessionTradeRequest) → SymbolInfoSessionTradeReply`

---

## 💬 Just the essentials

* **What it is.** Returns the **trade** session [from, to] for a symbol on a weekday and session index.
* **Why you need it.** Know when you can actually place/modify orders (vs merely watch quotes).
* **Sanity check.** `from() < to()` for an active tradable session.

---

## 🧱 Signature

```cpp
// Returns the trade session window. Throws ApiError / TransportError.
pb::SymbolInfoSessionTradeData symbolInfoSessionTrade(const pb::SymbolInfoSessionTradeRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoSessionTradeRequest {
  string    symbol        = 1;
  DayOfWeek day_of_week   = 2;   // SUNDAY=0 … SATURDAY=6
  uint32    session_index = 3;
}

SymbolInfoSessionTradeReply {
  oneof response {
    SymbolInfoSessionTradeData data  = 1;   // { Timestamp from; Timestamp to; }
    Error                      error = 2;
  }
}
```

---

## 🔽 Input

| Field           | C++ setter / type                       | Description                            |
| --------------- | --------------------------------------- | -------------------------------------- |
| `symbol`        | `set_symbol("EURUSD")`                  | Instrument.                            |
| `day_of_week`   | `set_day_of_week(pb::MONDAY)` — `DayOfWeek` | Weekday (`SUNDAY`(0) … `SATURDAY`(6)). |
| `session_index` | `set_session_index(0)` — uint32         | Which session of that day (0-based).   |

---

## ⬆️ Output — `SymbolInfoSessionTradeData`

| Field  | C++ accessor | C++ type                    | Description               |
| ------ | ------------ | --------------------------- | ------------------------- |
| `From` | `from()`     | `google::protobuf::Timestamp` | Tradable session start. |
| `To`   | `to()`       | `google::protobuf::Timestamp` | Tradable session end.   |

---

## 🧩 Notes & Tips

* **Trade ⊆ quote.** Tradable hours are usually within (or equal to) the quote session; compare with [SymbolInfoSessionQuote](./SymbolInfoSessionQuote.md).
* **Gate your orders.** Use this to avoid sending orders when the market is closed (retcode "market closed", `10018`).
* **Multiple sessions per day.** Walk `session_index` (0, 1, …).
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::SymbolInfoSessionTradeRequest r;
r.set_symbol("EURUSD");
r.set_day_of_week(pb::FRIDAY);
r.set_session_index(0);
pb::SymbolInfoSessionTradeData d = account.symbolInfoSessionTrade(r);
std::printf("Trade session: %lld .. %lld\n",
            (long long)d.from().seconds(), (long long)d.to().seconds());
```

---

## 📚 See Also

* [SymbolInfoSessionQuote](./SymbolInfoSessionQuote.md) — the quoting session window
* [OrderSend](../4.%20Trading_Operations/OrderSend.md) — placing orders (fails if the market is closed)
* **Friendly layer:** `MT5Service::sessionTrade(...)` returns a named `SessionTime` struct (UTC).
