# gRPC Streaming

Besides request/response calls, the gateway offers **server streams** — long-lived subscriptions
that push data (ticks, trade events, position profit, …). In CppMT5 a subscription runs on its own
background thread and delivers payloads to a callback.

## The model

```
sugar.service().subscribeTicks({...}, cb)  ──▶  ServiceStream handle   (you hold this)
                                                      │
                                                      ▼  background reader thread
                                                   cb(Tick)   cb(Tick)   cb(Tick) …
```

- Opening a stream returns a **handle** (`AccountStream` at the low level, `ServiceStream` at the
  friendly level). **Keep the handle alive** for as long as you want data.
- The callback runs on a dedicated reader thread. Destroying the handle — or calling `stop()` —
  cancels the stream and joins that thread.
- Streams do **not** go through the retry/reconnect machinery (there is nothing to retry on a
  long-lived stream); an error ends the stream and is reported via `lastError()`.

| Handle method | |
|---------------|--|
| `stop()` | cancel and join the reader thread (idempotent; also run by the destructor) |
| `running()` | false once the stream ended or was cancelled |
| `count()` | payloads delivered so far |
| `lastError()` | the error that ended it, or `""` if clean |

## Friendly streams (`MT5Service`)

Converted structs, times in UTC:

| Method | Payload |
|--------|---------|
| `subscribeTicks(symbols, cb)` | `Tick` |
| `streamTradeUpdates(cb)` | `TradeEvent` |
| `streamTransactions(cb)` | `TransactionEvent` |
| `streamPositionProfits(cb)` | `PositionProfitEvent` |
| `streamOpenedTickets(cb)` | `TicketsSnapshot` |

```cpp
using namespace metarpc;
MT5Service& svc = sugar.service();
svc.symbolSelect("EURUSD", true);   // a symbol must be selected to stream

auto stream = svc.subscribeTicks({"EURUSD"}, [](const Tick& t) {
    std::cout << t.symbol << "  bid=" << t.bid << " ask=" << t.ask << "  t=" << t.time << "\n";
});

// ... let it run; the handle keeps the subscription alive ...
std::this_thread::sleep_for(std::chrono::seconds(10));
stream->stop();
if (!stream->lastError().empty()) std::cerr << "ended: " << stream->lastError() << "\n";
```

## Low-level streams (`MT5Account`)

Raw protobuf payloads, raw server time:

```cpp
namespace pb = mt5_term_api;
pb::OnSymbolTickRequest req; req.add_symbol_names("EURUSD");
auto s = sugar.account().onSymbolTick(req, [](const pb::OnSymbolTickData& d) {
    if (d.has_symbol_tick()) std::cout << d.symbol_tick().bid() << "\n";
});
```

Also available: `onTrade`, `onTradeTransaction`, `onPositionProfit`,
`onPositionsAndPendingOrdersTickets`.

## Guidelines

- **Don't block the callback.** It runs on the reader thread and holds up the next payload — do
  slow work on another thread or push into a queue.
- **Exceptions thrown by your callback are swallowed** (they will not crash the reader thread), so
  handle errors inside it if you care about them.
- **One handle per subscription.** Store it; a dropped handle immediately cancels the stream.
- **Thread-safety:** the handle's accessors are safe from any thread; your own shared state is
  not — guard it (e.g. with an atomic or a mutex), as the demos do with `std::atomic<int>`.

## Re-subscribing after an error

Streams are **not** auto-reconnected (unlike unary calls). When a stream ends on an error,
`running()` turns `false` and `lastError()` holds the reason — it's up to you to open a new one:

```cpp
std::unique_ptr<ServiceStream> stream = svc.subscribeTicks({"EURUSD"}, onTick);

// a supervisor loop that re-subscribes if the stream drops
while (keepRunning) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (!stream->running()) {
        std::cerr << "stream ended: " << stream->lastError() << " — resubscribing\n";
        stream = svc.subscribeTicks({"EURUSD"}, onTick);   // old handle is replaced (and stopped)
    }
}
```

Because a dropped stream often follows a terminal hiccup, the next **unary** call will transparently
re-attach the terminal (`ConnectEx`), so a fresh subscription usually succeeds immediately.

## 📚 See also

* Low-level: [MT5Account · Streaming Methods](../MT5Account/7.%20Streaming_Methods/Streaming_Methods.Overview.md) (per-method pages)
* Friendly: [MT5Service · Streaming Methods](../MT5Service/6.%20Streaming_Methods.md)
* [Concurrency Model](SYNC_VS_ASYNC.md) — threads, deadlines, running work in parallel
