# ✅ Unsubscribing from Market Depth (DOM)

> **Request:** close a Level II / DOM subscription for a symbol (`MarketBookReleaseData`).

**API Information:**

* **Low-level API:** `MT5Account::marketBookRelease(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `MarketBookRelease` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `MarketBookRelease(MarketBookReleaseRequest) → MarketBookReleaseReply`

---

## 💬 Just the essentials

* **What it is.** Closes the DOM subscription opened by [MarketBookAdd](./MarketBookAdd.md).
* **Why you need it.** Free server resources when you stop watching the book.
* **Sanity check.** `closed_successfully()` is `true`.

---

## 🧱 Signature

```cpp
// Closes the Market Depth subscription for req.symbol().
// Throws ApiError / TransportError on failure.
pb::MarketBookReleaseData marketBookRelease(const pb::MarketBookReleaseRequest& req);
```

**Request / Reply:**

```protobuf
MarketBookReleaseRequest { string symbol = 1; }

MarketBookReleaseReply {
  oneof response {
    MarketBookReleaseData data  = 1;   // { bool closed_successfully = 1; }
    Error                 error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type              | Description                     |
| -------- | ------------------------------ | ------------------------------- |
| `symbol` | `set_symbol("EURUSD")` — string | Symbol to unsubscribe DOM for. |

---

## ⬆️ Output — `MarketBookReleaseData`

| Field                | C++ accessor           | C++ type | Description                      |
| -------------------- | ---------------------- | -------- | ------------------------------- |
| `ClosedSuccessfully` | `closed_successfully()`| `bool`   | `true` if the subscription closed. |

---

## 🧩 Notes & Tips

* **Always pair with [MarketBookAdd](./MarketBookAdd.md).** Leaking subscriptions wastes server resources.
* **Idempotent-ish.** Releasing a symbol you never subscribed simply returns `false`.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::MarketBookReleaseRequest r; r.set_symbol("EURUSD");
std::printf("DOM closed: %s\n", account.marketBookRelease(r).closed_successfully() ? "yes" : "no");
```

### RAII guard pattern

```cpp
struct DomGuard {
    MT5Account& a; std::string sym; bool ok = false;
    DomGuard(MT5Account& acc, std::string s) : a(acc), sym(std::move(s)) {
        pb::MarketBookAddRequest r; r.set_symbol(sym);
        ok = a.marketBookAdd(r).opened_successfully();
    }
    ~DomGuard() {
        if (ok) { pb::MarketBookReleaseRequest r; r.set_symbol(sym); a.marketBookRelease(r); }
    }
};
// Usage: { DomGuard g(account, "EURUSD"); /* poll marketBookGet */ }  // auto-released
```

---

## 📚 See Also

* [MarketBookAdd](./MarketBookAdd.md) — subscribe
* [MarketBookGet](./MarketBookGet.md) — read the book
* **Friendly layer:** `MT5Service::marketBookRelease("EURUSD")`.
