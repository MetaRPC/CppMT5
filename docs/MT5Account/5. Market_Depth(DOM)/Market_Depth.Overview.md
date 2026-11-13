# MT5Account · Market Depth (DOM) — Overview

> Level II quotes / order book: subscribe, read, unsubscribe. A three-step lifecycle on the `MarketInfo` service.

## 📁 What lives here

* **[MarketBookAdd](./MarketBookAdd.md)** — **subscribe** to a symbol's order book (`opened_successfully`).
* **[MarketBookGet](./MarketBookGet.md)** — **read** the current book (`mql_book_infos` of `MrpcMqlBookInfo`).
* **[MarketBookRelease](./MarketBookRelease.md)** — **unsubscribe** (`closed_successfully`).

---

## 🧭 Plain English

The order book is a **subscription**, not a one-shot read:

1. `marketBookAdd("EURUSD")` → start receiving depth.
2. `marketBookGet("EURUSD")` → snapshot the current levels (poll as needed).
3. `marketBookRelease("EURUSD")` → stop and free resources.

> Skipping step 1 means step 2 returns an empty book. Skipping step 3 leaks a subscription.

---

## Quick choose

| If you need…              | Use                 | Returns                       |
| ------------------------- | ------------------- | ----------------------------- |
| Start a DOM subscription  | `marketBookAdd`     | `bool` (`opened_successfully()`) |
| Read the current book     | `marketBookGet`     | `MrpcMqlBookInfo[]`           |
| Stop the subscription     | `marketBookRelease` | `bool` (`closed_successfully()`) |

---

## 🧱 Book level — `MrpcMqlBookInfo`

| Accessor        | Type       | Description                                                  |
| --------------- | ---------- | ----------------------------------------------------------- |
| `type()`        | `BookType` | `BOOK_TYPE_SELL`(0), `BOOK_TYPE_BUY`(1), `BOOK_TYPE_SELL_MARKET`(2), `BOOK_TYPE_BUY_MARKET`(3). |
| `price()`       | `double`   | Level price.                                                |
| `volume()`      | `int64`    | Level volume (integer).                                     |
| `volume_real()` | `double`   | Level volume (high precision).                              |

---

## ❌ Cross-refs & gotchas

* **Not every symbol/broker has DOM.** `opened_successfully() == false` means no Level II data for that instrument.
* **Poll, don't spin.** `marketBookGet` is a snapshot — poll at a sensible interval instead of a tight loop.
* **Clean up.** Prefer an RAII guard (see [MarketBookRelease](./MarketBookRelease.md)) so a subscription is always released.
* **Select the symbol first** ([SymbolSelect](../2.%20Symbol_Information/SymbolSelect.md)).

---

## 🟢 Minimal lifecycle

```cpp
pb::MarketBookAddRequest add; add.set_symbol("EURUSD");
if (account.marketBookAdd(add).opened_successfully()) {
    pb::MarketBookGetRequest get; get.set_symbol("EURUSD");
    for (const auto& lvl : account.marketBookGet(get).mql_book_infos())
        std::printf("%d %.5f x%lld\n", (int)lvl.type(), lvl.price(), (long long)lvl.volume());

    pb::MarketBookReleaseRequest rel; rel.set_symbol("EURUSD");
    account.marketBookRelease(rel);
}
```

---

## See also

* **Symbols:** [SymbolInfoTick](../2.%20Symbol_Information/SymbolInfoTick.md) — top-of-book bid/ask
* **Streaming:** [OnSymbolTick](../7.%20Streaming_Methods/OnSymbolTick.md) — push-based quotes
* **Friendly layer:** `MT5Service` DOM helpers return `vector<BookEntry>`.
