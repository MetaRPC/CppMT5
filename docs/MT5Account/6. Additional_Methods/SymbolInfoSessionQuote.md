# ✅ Getting Quote Session Times

> **Request:** the quoting session window for a symbol on a given weekday (`SymbolInfoSessionQuoteData`).

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoSessionQuote(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoSessionQuote` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoSessionQuote(SymbolInfoSessionQuoteRequest) → SymbolInfoSessionQuoteReply`

---

## 💬 Just the essentials

* **What it is.** Returns the **quote** session [from, to] for a symbol on a weekday and session index.
* **Why you need it.** Know when a symbol is quoting (prices update) vs silent.
* **Sanity check.** `from() < to()` for an active session; an empty/zero window means no session.

---

## 🧱 Signature

```cpp
// Returns the quote session window. Throws ApiError / TransportError.
pb::SymbolInfoSessionQuoteData symbolInfoSessionQuote(const pb::SymbolInfoSessionQuoteRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoSessionQuoteRequest {
  string    symbol        = 1;
  DayOfWeek day_of_week   = 2;   // SUNDAY=0 … SATURDAY=6
  uint32    session_index = 3;   // 0-based; a day can have multiple sessions
}

SymbolInfoSessionQuoteReply {
  oneof response {
    SymbolInfoSessionQuoteData data  = 1;   // { Timestamp from; Timestamp to; }
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

## ⬆️ Output — `SymbolInfoSessionQuoteData`

| Field  | C++ accessor | C++ type                    | Description                 |
| ------ | ------------ | --------------------------- | --------------------------- |
| `From` | `from()`     | `google::protobuf::Timestamp` | Session start.            |
| `To`   | `to()`       | `google::protobuf::Timestamp` | Session end.              |

Read `.seconds()` for epoch seconds (time-of-day within the session day).

---

## 🧩 Notes & Tips

* **Multiple sessions per day.** Increment `session_index` (0, 1, …) until the call stops returning a valid window.
* **Quote vs trade sessions differ.** Prices may quote outside of tradable hours — see [SymbolInfoSessionTrade](./SymbolInfoSessionTrade.md).
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::SymbolInfoSessionQuoteRequest r;
r.set_symbol("EURUSD");
r.set_day_of_week(pb::MONDAY);
r.set_session_index(0);
pb::SymbolInfoSessionQuoteData d = account.symbolInfoSessionQuote(r);
std::printf("Quote session: %lld .. %lld\n",
            (long long)d.from().seconds(), (long long)d.to().seconds());
```

---

## 📚 See Also

* [SymbolInfoSessionTrade](./SymbolInfoSessionTrade.md) — the tradable session window
* [SymbolInfoInteger](../2.%20Symbol_Information/SymbolInfoInteger.md) — `SYMBOL_TRADE_MODE`, session counts
* **Friendly layer:** `MT5Service::sessionQuote(...)` returns a named `SessionTime` struct (UTC).
