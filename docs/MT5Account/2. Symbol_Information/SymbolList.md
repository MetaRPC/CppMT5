# ✅ Listing All Symbol Names

> **Request:** every symbol name in one call (`SymbolListData`) — no index loop needed.

**API Information:**

* **Low-level API:** `MT5Account::symbolList(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolList` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolList(SymbolListRequest) → SymbolListReply`

---

## 💬 Just the essentials

* **What it is.** Returns a repeated list of symbol names in a single response.
* **Why you need it.** The convenient alternative to [SymbolsTotal](./SymbolsTotal.md) + [SymbolName](./SymbolName.md) in a loop.
* **Sanity check.** `symbols_size()` > 0.

> This is a **CppMT5 convenience RPC** — it rolls the count-then-index pattern into one call.

---

## 🧱 Signature

```cpp
// Returns the list of symbol names. Request defaults to {}.
// Throws ApiError / TransportError on failure.
pb::SymbolListData symbolList(const pb::SymbolListRequest& req = {});
```

**Request / Reply:**

```protobuf
SymbolListRequest {}   // (empty)

SymbolListReply {
  oneof response {
    SymbolListData data  = 1;   // { repeated string symbols = 1; }
    Error          error = 2;
  }
}
```

---

## 🔽 Input

| Parameter | Type                         | Description                                 |
| --------- | ---------------------------- | ------------------------------------------- |
| `req`     | `const pb::SymbolListRequest&` | Empty request. Optional — defaults to `{}`. |

---

## ⬆️ Output — `SymbolListData`

| Field     | C++ accessor                     | C++ type                | Description                 |
| --------- | -------------------------------- | ----------------------- | --------------------------- |
| `Symbols` | `symbols()` / `symbols(i)` / `symbols_size()` | repeated `string` | All symbol names.           |

---

## 🧩 Notes & Tips

* **Iterate with a range-for:** `for (const auto& s : data.symbols()) { ... }`.
* **Cheaper than the loop:** one round-trip instead of `1 + total` calls.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Print all symbols

```cpp
pb::SymbolListData d = account.symbolList();
std::printf("%d symbols:\n", d.symbols_size());
for (const auto& name : d.symbols())
    std::printf("  %s\n", name.c_str());
```

### 2) Filter to a prefix

```cpp
pb::SymbolListData d = account.symbolList();
for (const auto& name : d.symbols())
    if (name.rfind("EUR", 0) == 0)      // starts with "EUR"
        std::printf("%s\n", name.c_str());
```

---

## 📚 See Also

* [SymbolsTotal](./SymbolsTotal.md) / [SymbolName](./SymbolName.md) — the index-based alternative
* [SymbolSelect](./SymbolSelect.md) — add/remove a symbol from Market Watch
* [SymbolExist](./SymbolExist.md) — check one symbol without listing everything
