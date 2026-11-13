# ✅ Selecting a Symbol in Market Watch

> **Request:** add a symbol to, or remove it from, Market Watch (`SymbolSelectData`).

**API Information:**

* **Low-level API:** `MT5Account::symbolSelect(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolSelect` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolSelect(SymbolSelectRequest) → SymbolSelectReply`

---

## 💬 Just the essentials

* **What it is.** Enables (or disables) a symbol in Market Watch.
* **Why you need it.** Quotes, ticks and DOM only flow for **selected** symbols — select before you quote or trade an unusual symbol.
* **Sanity check.** `success()` is `true`.

---

## 🎯 Purpose

* Make a symbol quotable before [SymbolInfoTick](./SymbolInfoTick.md) / [OnSymbolTick](../7.%20Streaming_Methods/OnSymbolTick.md).
* Clean up Market Watch by removing symbols you no longer track.

---

## 🧱 Signature

```cpp
// Adds (select=true) or removes (select=false) req.symbol() from Market Watch.
// Throws ApiError / TransportError on failure.
pb::SymbolSelectData symbolSelect(const pb::SymbolSelectRequest& req);
```

**Request / Reply:**

```protobuf
SymbolSelectRequest {
  string symbol = 1;
  bool   select = 2;   // true = add to Market Watch; false = remove
}

SymbolSelectReply {
  oneof response {
    SymbolSelectData data  = 1;   // { bool success = 1; }
    Error            error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type              | Description                          |
| -------- | ------------------------------ | ------------------------------------ |
| `symbol` | `set_symbol("GBPJPY")` — string | The symbol to add/remove.            |
| `select` | `set_select(bool)` — bool      | `true` = add to Market Watch; `false` = remove. |

---

## ⬆️ Output — `SymbolSelectData`

| Field     | C++ accessor | C++ type | Description                         |
| --------- | ------------ | -------- | ----------------------------------- |
| `Success` | `success()`  | `bool`   | `true` if the operation succeeded.  |

---

## 🧩 Notes & Tips

* **A symbol in use cannot be removed.** Removing a symbol that has open positions or orders will fail.
* **Select is idempotent.** Selecting an already-selected symbol just returns success.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Ensure a symbol is quotable

```cpp
pb::SymbolSelectRequest r; r.set_symbol("GBPJPY"); r.set_select(true);
if (account.symbolSelect(r).success()) {
    pb::SymbolInfoTickRequest tr; tr.set_symbol("GBPJPY");
    pb::MrpcMqlTick t = account.symbolInfoTick(tr);
    std::printf("GBPJPY %.3f / %.3f\n", t.bid(), t.ask());
}
```

### 2) Remove a symbol from Market Watch

```cpp
pb::SymbolSelectRequest r; r.set_symbol("GBPJPY"); r.set_select(false);
std::printf("Removed: %s\n", account.symbolSelect(r).success() ? "yes" : "no");
```

---

## 📚 See Also

* [SymbolExist](./SymbolExist.md) — check a symbol exists before selecting it
* [SymbolInfoTick](./SymbolInfoTick.md) — quote a selected symbol
* [SymbolIsSynchronized](./SymbolIsSynchronized.md) — confirm data is ready after selecting
