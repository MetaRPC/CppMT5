# ✅ Getting a Symbol Name by Index

> **Request:** the name of the symbol at a given index (`SymbolNameData`), from the full list or the Market Watch list.

**API Information:**

* **Low-level API:** `MT5Account::symbolName(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolName` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolName(SymbolNameRequest) → SymbolNameReply`

---

## 💬 Just the essentials

* **What it is.** Resolves an **index** to a **symbol name**.
* **Why you need it.** To enumerate symbols together with [SymbolsTotal](./SymbolsTotal.md).
* **Sanity check.** `index = 0` returns a valid, non-empty name.

---

## 🧱 Signature

```cpp
// Returns the symbol name at req.index(). Throws ApiError / TransportError.
pb::SymbolNameData symbolName(const pb::SymbolNameRequest& req);
```

**Request / Reply:**

```protobuf
SymbolNameRequest {
  int32 index    = 1;
  bool  selected = 2;   // true = index into Market Watch; false = index into the full list
}

SymbolNameReply {
  oneof response {
    SymbolNameData data  = 1;   // { string name = 1; }
    Error          error = 2;
  }
}
```

---

## 🔽 Input

| Field      | C++ setter / type            | Description                                             |
| ---------- | ---------------------------- | ------------------------------------------------------ |
| `index`    | `set_index(int32)` — int32   | Zero-based index into the chosen list.                 |
| `selected` | `set_selected(bool)` — bool  | `true` → index into the Market Watch (selected) list; `false` → index into the full server list. |

> Keep `selected` consistent with the `mode` you passed to [SymbolsTotal](./SymbolsTotal.md), or your index bounds won't match.

---

## ⬆️ Output — `SymbolNameData`

| Field  | C++ accessor | C++ type             | Description               |
| ------ | ------------ | -------------------- | ------------------------- |
| `Name` | `name()`     | `const std::string&` | Symbol name at the index. |

---

## 🧩 Notes & Tips

* **Bounds:** valid indices are `0 … total-1` for the matching list; out-of-range indices throw `ApiError`.
* **Prefer [SymbolList](./SymbolList.md)** when you want every name — it avoids a call per index.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) First symbol

```cpp
pb::SymbolNameRequest r; r.set_index(0); r.set_selected(true);
std::printf("First MW symbol: %s\n", account.symbolName(r).name().c_str());
```

### 2) Enumerate the Market Watch list

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

* [SymbolsTotal](./SymbolsTotal.md) — how many symbols there are to index
* [SymbolList](./SymbolList.md) — all names at once
* [SymbolSelect](./SymbolSelect.md) — add a symbol to Market Watch so it appears in the selected list
