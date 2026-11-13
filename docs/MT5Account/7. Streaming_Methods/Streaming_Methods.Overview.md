# MT5Account · Streaming Methods — Overview

> Real-time server streams: ticks, trade events, the transaction log, profit updates, and ticket snapshots. All run on the `SubscriptionService` and deliver to a callback on a background thread.

## 📁 What lives here

* **[OnSymbolTick](./OnSymbolTick.md)** — live ticks for one or more symbols.
* **[OnTrade](./OnTrade.md)** — position/order change events (opened, closed, modified).
* **[OnTradeTransaction](./OnTradeTransaction.md)** — detailed transaction log (request + transaction + result).
* **[OnPositionProfit](./OnPositionProfit.md)** — periodic profit/loss updates (timer-driven).
* **[OnPositionsAndPendingOrdersTickets](./OnPositionsAndPendingOrdersTickets.md)** — periodic ticket snapshots (lightweight).

---

## 🧭 The streaming model (read this first)

Every `onX(req, callback)` returns a **`std::unique_ptr<AccountStream>`**:

```cpp
auto stream = account.onSymbolTick(req, [](const pb::OnSymbolTickData& d){ /* ... */ });
// ... stream is live on a background thread ...
stream->stop();         // end the subscription
```

* The SDK reads the server stream on a **dedicated background thread** and calls your `callback` for each payload.
* **Keep the handle alive.** Destroying the `unique_ptr` (or calling `stop()`) cancels the stream.
* **The callback runs on the reader thread.** Don't block it — push slow work elsewhere and guard shared state (`std::atomic` / mutex).
* **Exceptions thrown by the callback are swallowed** — handle errors inside it.
* **Payload times are raw broker server time** (many payloads also carry a `*_utc` field).

`AccountStream` handle API: `stop()`, `running()`, `count()` (payloads received so far), `lastError()`.

---

## Quick choose

| If you want…                        | Use                                   | Shape                        |
| ----------------------------------- | ------------------------------------- | ---------------------------- |
| Live prices                         | `onSymbolTick`                        | event per tick               |
| React to fills/closes/modifies      | `onTrade`                             | delta events                 |
| Full audit of each transaction      | `onTradeTransaction`                  | request + transaction + result |
| Live P/L of positions               | `onPositionProfit`                    | timer, new/updated/deleted   |
| Cheap "what tickets exist" feed     | `onPositionsAndPendingOrdersTickets`  | timer, full ticket snapshot  |

---

## ❌ Cross-refs & gotchas

* **Deltas vs snapshots.** `onTrade` / `onPositionProfit` report *changes*; `onPositionsAndPendingOrdersTickets` sends a *full* ticket list each tick (diff it yourself).
* **One stream, many symbols.** For ticks, prefer a single `onSymbolTick` with several symbols over many single-symbol streams.
* **Pick a sensible timer.** `onPositionProfit` / `onPositionsAndPendingOrdersTickets` take `timer_period_milliseconds` — 500–2000 ms is usually enough; set `ignore_empty_data` where available.
* **Lifetime bugs are the #1 pitfall.** If a stream "stops working", check you still hold its handle.

---

## 🟢 Minimal pattern

```cpp
#include <thread>
#include <chrono>

pb::OnSymbolTickRequest req;
req.add_symbol_names("EURUSD");

auto stream = account.onSymbolTick(req, [](const pb::OnSymbolTickData& d) {
    std::printf("%s %.5f/%.5f\n",
                d.symbol_tick().symbol().c_str(),
                d.symbol_tick().bid(), d.symbol_tick().ask());
});

std::this_thread::sleep_for(std::chrono::seconds(10));
stream->stop();
```

---

## See also

* **Guide:** [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md) — lifecycle, threading, backpressure
* **Concurrency:** [Concurrency Model](../../All_Guides/SYNC_VS_ASYNC.md) — unary vs stream, running work in parallel
* **Friendly layer:** `MT5Service` stream helpers deliver named event structs with UTC times.
