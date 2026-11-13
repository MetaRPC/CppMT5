# ✅ Checking Symbol Data Synchronization

> **Request:** is a symbol's data synchronized with the server (`SymbolIsSynchronizedData`)?

**API Information:**

* **Low-level API:** `MT5Account::symbolIsSynchronized(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolIsSynchronized` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolIsSynchronized(SymbolIsSynchronizedRequest) → SymbolIsSynchronizedReply`

---

## 💬 Just the essentials

* **What it is.** Tells you whether the terminal has finished synchronizing a symbol's specification and quotes with the server.
* **Why you need it.** Right after selecting a fresh symbol, its parameters/quotes may not be ready yet — this confirms they are.
* **Sanity check.** After [SymbolSelect](./SymbolSelect.md) + a short wait, `synchronized()` becomes `true`.

---

## 🧱 Signature

```cpp
// Returns whether req.symbol()'s data is synchronized. Throws ApiError / TransportError.
pb::SymbolIsSynchronizedData symbolIsSynchronized(const pb::SymbolIsSynchronizedRequest& req);
```

**Request / Reply:**

```protobuf
SymbolIsSynchronizedRequest { string symbol = 1; }

SymbolIsSynchronizedReply {
  oneof response {
    SymbolIsSynchronizedData data  = 1;   // { bool synchronized = 1; }
    Error                    error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type              | Description            |
| -------- | ------------------------------ | ---------------------- |
| `symbol` | `set_symbol("EURUSD")` — string | The symbol to check.   |

---

## ⬆️ Output — `SymbolIsSynchronizedData`

| Field          | C++ accessor     | C++ type | Description                               |
| -------------- | ---------------- | -------- | ----------------------------------------- |
| `Synchronized` | `synchronized()` | `bool`   | `true` if the symbol's data is ready.     |

---

## 🧩 Notes & Tips

* **Use after selecting a new symbol.** Freshly selected symbols may need a moment to synchronize before their [SymbolInfoDouble](./SymbolInfoDouble.md) parameters are valid.
* **Poll gently.** If `false`, wait a short interval and check again rather than spinning.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Wait until a symbol is ready

```cpp
#include <thread>
#include <chrono>

bool waitSynced(MT5Account& a, const std::string& symbol, int tries = 10) {
    for (int i = 0; i < tries; ++i) {
        pb::SymbolIsSynchronizedRequest r; r.set_symbol(symbol);
        if (a.symbolIsSynchronized(r).synchronized()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

// Usage: select, then wait before reading parameters
pb::SymbolSelectRequest sel; sel.set_symbol("USDMXN"); sel.set_select(true);
account.symbolSelect(sel);
if (waitSynced(account, "USDMXN"))
    std::puts("USDMXN ready");
```

---

## 📚 See Also

* [SymbolSelect](./SymbolSelect.md) — select the symbol first
* [SymbolInfoDouble](./SymbolInfoDouble.md) — read parameters once synchronized
* [SymbolExist](./SymbolExist.md) — confirm the symbol exists at all
