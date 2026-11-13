# ✅ Reading the Order Book (DOM)

> **Request:** the current Level II order book for a subscribed symbol (`MarketBookGetData`).

**API Information:**

* **Low-level API:** `MT5Account::marketBookGet(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `MarketBookGet` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `MarketBookGet(MarketBookGetRequest) → MarketBookGetReply`

---

## 💬 Just the essentials

* **What it is.** Returns the current snapshot of the order book: a list of bid/ask levels with price and volume.
* **Why you need it.** Gauge liquidity and depth beyond the top-of-book bid/ask.
* **Prerequisite.** You must have called [MarketBookAdd](./MarketBookAdd.md) for the symbol first.

---

## 🧱 Signature

```cpp
// Returns the current order book for req.symbol().
// Throws ApiError / TransportError on failure.
pb::MarketBookGetData marketBookGet(const pb::MarketBookGetRequest& req);
```

**Request / Reply:**

```protobuf
MarketBookGetRequest { string symbol = 1; }

MarketBookGetReply {
  oneof response {
    MarketBookGetData data  = 1;   // { repeated MrpcMqlBookInfo mql_book_infos = 1; }
    Error             error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type              | Description                          |
| -------- | ------------------------------ | ------------------------------------ |
| `symbol` | `set_symbol("EURUSD")` — string | Symbol (must be subscribed via `marketBookAdd`). |

---

## ⬆️ Output — `MarketBookGetData`

| Field          | C++ accessor        | Element type      | Description                |
| -------------- | ------------------- | ----------------- | -------------------------- |
| `MqlBookInfos` | `mql_book_infos()`  | `MrpcMqlBookInfo` | Order book levels.         |

Use `mql_book_infos_size()`, `mql_book_infos(i)`, or range-for.

### `MrpcMqlBookInfo` — level fields

| Accessor        | Type       | Description                                |
| --------------- | ---------- | ------------------------------------------ |
| `type()`        | `BookType` | Level side: `BOOK_TYPE_SELL`(0), `BOOK_TYPE_BUY`(1), `BOOK_TYPE_SELL_MARKET`(2), `BOOK_TYPE_BUY_MARKET`(3). |
| `price()`       | `double`   | Price at this level.                        |
| `volume()`      | `int64`    | Volume at this level (integer).             |
| `volume_real()` | `double`   | Volume with greater precision.              |

---

## 🧩 Notes & Tips

* **Levels come in both sides.** Filter by `type()` to split asks (`SELL`) from bids (`BUY`).
* **Snapshot, not a stream.** Poll `marketBookGet` for updates, or use tick streaming for top-of-book.
* **Empty book?** If you didn't call [MarketBookAdd](./MarketBookAdd.md) (or the symbol has no DOM), the list may be empty.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::MarketBookAddRequest add; add.set_symbol("EURUSD");
account.marketBookAdd(add);

pb::MarketBookGetRequest r; r.set_symbol("EURUSD");
pb::MarketBookGetData book = account.marketBookGet(r);

for (const auto& lvl : book.mql_book_infos()) {
    const char* side =
        lvl.type() == pb::BOOK_TYPE_BUY || lvl.type() == pb::BOOK_TYPE_BUY_MARKET ? "BID" : "ASK";
    std::printf("%s  %.5f  x%lld\n", side, lvl.price(), (long long)lvl.volume());
}

pb::MarketBookReleaseRequest rel; rel.set_symbol("EURUSD");
account.marketBookRelease(rel);
```

---

## 📚 See Also

* [MarketBookAdd](./MarketBookAdd.md) — subscribe first
* [MarketBookRelease](./MarketBookRelease.md) — unsubscribe when done
* [SymbolInfoTick](../2.%20Symbol_Information/SymbolInfoTick.md) — top-of-book bid/ask only
* **Friendly layer:** `MT5Service::marketBookGet("EURUSD")` returns a `vector<BookEntry>`.
