# ✅ Checking Whether a Symbol Exists

> **Request:** does a symbol exist on the server (`SymbolExistData`), and is it custom?

**API Information:**

* **Low-level API:** `MT5Account::symbolExist(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolExist` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolExist(SymbolExistRequest) → SymbolExistReply`

---

## 💬 Just the essentials

* **What it is.** Tells you whether a symbol name is valid on the server, and whether it is a custom (synthetic) symbol.
* **Why you need it.** Validate user input before selecting/quoting/trading a symbol.
* **Sanity check.** `symbolExist("EURUSD").exists()` is `true`.

---

## 🧱 Signature

```cpp
// Returns existence/custom flags for req.name(). Throws ApiError / TransportError.
pb::SymbolExistData symbolExist(const pb::SymbolExistRequest& req);
```

**Request / Reply:**

```protobuf
SymbolExistRequest { string name = 1; }

SymbolExistReply {
  oneof response {
    SymbolExistData data  = 1;   // { bool exists = 1; bool is_custom = 2; }
    Error           error = 2;
  }
}
```

---

## 🔽 Input

| Field  | C++ setter / type             | Description             |
| ------ | ----------------------------- | ----------------------- |
| `name` | `set_name("EURUSD")` — string | The symbol name to check. |

> Note: this request uses `name`, not `symbol`.

---

## ⬆️ Output — `SymbolExistData`

| Field      | C++ accessor  | C++ type | Description                               |
| ---------- | ------------- | -------- | ----------------------------------------- |
| `Exists`   | `exists()`    | `bool`   | `true` if the symbol exists on the server.|
| `IsCustom` | `is_custom()` | `bool`   | `true` if it is a custom/synthetic symbol.|

---

## 🧩 Notes & Tips

* **Existence ≠ selected.** A symbol can exist on the server but not be in Market Watch — use [SymbolSelect](./SymbolSelect.md) to quote it.
* **Cheap guard.** Call this before building a request with user-supplied symbols to turn a server error into a clean boolean.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Validate before trading

```cpp
pb::SymbolExistRequest r; r.set_name("EURUSD");
pb::SymbolExistData d = account.symbolExist(r);
if (!d.exists()) { std::puts("Unknown symbol"); return; }
std::printf("EURUSD exists (custom: %s)\n", d.is_custom() ? "yes" : "no");
```

### 2) A boolean helper

```cpp
bool symbolExists(MT5Account& a, const std::string& name) {
    pb::SymbolExistRequest r; r.set_name(name);
    return a.symbolExist(r).exists();
}
```

---

## 📚 See Also

* [SymbolSelect](./SymbolSelect.md) — add an existing symbol to Market Watch
* [SymbolList](./SymbolList.md) — list all valid names
* [SymbolIsSynchronized](./SymbolIsSynchronized.md) — check the symbol's data is ready
