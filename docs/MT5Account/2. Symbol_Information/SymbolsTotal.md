# ✅ Counting Available Symbols

> **Request:** the number of symbols (`SymbolsTotalData`) available on the server, optionally only those in Market Watch.

**API Information:**

* **Low-level API:** `MT5Account::symbolsTotal(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolsTotal` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolsTotal(SymbolsTotalRequest) → SymbolsTotalReply`

---

## 💬 Just the essentials

* **What it is.** Returns a single count of symbols.
* **Why you need it.** To iterate symbols by index with [SymbolName](./SymbolName.md), or to size a UI list.
* **Sanity check.** The count is > 0 on any live server.

---

## 🧱 Signature

```cpp
// Returns the symbol count. Throws ApiError / TransportError on failure.
pb::SymbolsTotalData symbolsTotal(const pb::SymbolsTotalRequest& req);
```

**Request / Reply:**

```protobuf
SymbolsTotalRequest { bool mode = 1; }   // true = only Market Watch selected; false = all

SymbolsTotalReply {
  oneof response {
    SymbolsTotalData data  = 1;   // { int32 total = 1; }
    Error            error = 2;
  }
}
```

---

## 🔽 Input

| Field  | C++ setter / type       | Description                                                  |
| ------ | ----------------------- | ----------------------------------------------------------- |
| `mode` | `set_mode(bool)` — bool | `true` → count only symbols selected in Market Watch; `false` → count all symbols on the server. |

---

## ⬆️ Output — `SymbolsTotalData`

| Field   | C++ accessor | C++ type  | Description                |
| ------- | ------------ | --------- | -------------------------- |
| `Total` | `total()`    | `int32_t` | Number of symbols counted. |

---

## 🧩 Notes & Tips

* **Pair with [SymbolName](./SymbolName.md):** iterate `index = 0 … total-1` to enumerate symbols.
* **`mode` matters:** the "all" count can be thousands; the Market Watch count is usually a short, curated list.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Count all vs selected

```cpp
auto count = [&](bool onlyMW) {
    pb::SymbolsTotalRequest r; r.set_mode(onlyMW);
    return account.symbolsTotal(r).total();
};
std::printf("All: %d | Market Watch: %d\n", count(false), count(true));
```

### 2) Enumerate every selected symbol

```cpp
pb::SymbolsTotalRequest tr; tr.set_mode(true);
int32_t n = account.symbolsTotal(tr).total();

for (int32_t i = 0; i < n; ++i) {
    pb::SymbolNameRequest nr; nr.set_index(i); nr.set_selected(true);
    std::printf("%2d: %s\n", i, account.symbolName(nr).name().c_str());
}
```

---

## 📚 See Also

* [SymbolName](./SymbolName.md) — resolve an index to a symbol name
* [SymbolList](./SymbolList.md) — get all names in one call (no index loop)
* [SymbolSelect](./SymbolSelect.md) — add/remove a symbol from Market Watch
