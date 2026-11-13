# ✅ Subscribing to Market Depth (DOM)

> **Request:** open a Level II / Depth-of-Market subscription for a symbol (`MarketBookAddData`).

**API Information:**

* **Low-level API:** `MT5Account::marketBookAdd(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `MarketBookAdd` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `MarketBookAdd(MarketBookAddRequest) → MarketBookAddReply`

---

## 💬 Just the essentials

* **What it is.** Opens a DOM (order book) subscription for one symbol on the server side.
* **Why you need it.** You must subscribe **before** [MarketBookGet](./MarketBookGet.md) returns data.
* **Sanity check.** `opened_successfully()` is `true`.

---

## 🧱 Signature

```cpp
// Opens a Market Depth subscription for req.symbol().
// Throws ApiError / TransportError on failure.
pb::MarketBookAddData marketBookAdd(const pb::MarketBookAddRequest& req);
```

**Request / Reply:**

```protobuf
MarketBookAddRequest { string symbol = 1; }

MarketBookAddReply {
  oneof response {
    MarketBookAddData data  = 1;   // { bool opened_successfully = 1; }
    Error             error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type              | Description                      |
| -------- | ------------------------------ | -------------------------------- |
| `symbol` | `set_symbol("EURUSD")` — string | Symbol to subscribe DOM for.    |

---

## ⬆️ Output — `MarketBookAddData`

| Field                | C++ accessor           | C++ type | Description                      |
| -------------------- | ---------------------- | -------- | ------------------------------- |
| `OpenedSuccessfully` | `opened_successfully()`| `bool`   | `true` if the subscription opened. |

---

## 🧩 Notes & Tips

* **Not all symbols/brokers provide DOM.** If `opened_successfully()` is `false`, the instrument likely has no Level II data.
* **Always release.** Pair every `marketBookAdd` with a [MarketBookRelease](./MarketBookRelease.md) when done.
* **Select the symbol first** ([SymbolSelect](../2.%20Symbol_Information/SymbolSelect.md)).
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::MarketBookAddRequest r; r.set_symbol("EURUSD");
if (account.marketBookAdd(r).opened_successfully()) {
    // ... poll MarketBookGet ...
    pb::MarketBookReleaseRequest rel; rel.set_symbol("EURUSD");
    account.marketBookRelease(rel);
} else {
    std::puts("DOM not available for this symbol");
}
```

---

## 📚 See Also

* [MarketBookGet](./MarketBookGet.md) — read the order book once subscribed
* [MarketBookRelease](./MarketBookRelease.md) — unsubscribe
* **Friendly layer:** `MT5Service::marketBookAdd("EURUSD")`.
