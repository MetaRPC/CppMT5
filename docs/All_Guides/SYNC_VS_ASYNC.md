# Concurrency Model

CppMT5 has **no async/await**. There are exactly two shapes of call, and plain C++ threads are all
you need to run things concurrently.

## Two shapes of call

| Shape | What it is | How you consume it |
|-------|-----------|--------------------|
| **Unary (request/response)** | `getBalance()`, `symbolInfoTick()`, `orderSend()` … | an ordinary **blocking** function; it returns `(value)` or throws |
| **Server stream** | `subscribeTicks()`, `streamTradeUpdates()` … | returns a handle; the SDK reads the stream on a **background thread** and calls your callback |

### Unary — synchronous

```cpp
double bal = sugar.getBalance();        // blocks until the reply (or the deadline), then returns
auto    t  = sugar.getPriceInfo("EURUSD");
```

There is nothing to `await`; the gRPC round-trip happens inside the function, with the deadline
from `ClientOptions`. Failures are exceptions (`ApiError` / `TransportError`).

### Streaming — one background thread per subscription

See [gRPC Streaming](GRPC_STREAM_MANAGEMENT.md). The callback runs on the stream's reader thread;
keep the handle alive, call `stop()` to end it.

## Running work concurrently

Use `std::thread` / `std::async` yourself — fan out independent calls:

```cpp
#include <future>

auto bid = std::async(std::launch::async, [&]{ return sugar.getBid("EURUSD"); });
auto ask = std::async(std::launch::async, [&]{ return sugar.getAsk("GBPUSD"); });
std::cout << bid.get() << " " << ask.get() << "\n";
```

Or run a management loop in the background while the main flow continues:

```cpp
std::atomic<bool> stop{false};
std::thread monitor([&]{
    while (!stop) {
        if (sugar.getMarginLevel() < 150 && sugar.countOpenPositions() > 0)
            sugar.closeAllPositions();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
});
// ... later ...
stop = true; monitor.join();
```

## Thread-safety rules

- **One `MT5Account`/`MT5Service`/`MT5Sugar` owns one gRPC channel.** Unary calls may be issued
  from multiple threads, but `connect()` / `reconnect()` / `close()` mutate the channel and must
  not race with in-flight calls — do them from one thread, before fanning out.
- **Stream callbacks run on their own threads.** Guard any state you share with the main thread
  (the demos use `std::atomic`), and don't block the callback — push slow work elsewhere.
- **Prefer one stream with many symbols** over many single-symbol streams where the API allows it
  (`subscribeTicks({"EURUSD","GBPUSD"}, cb)`).

## Deadlines & timeouts

There is no per-call `ctx`/cancellation token. Each unary call runs against a **per-attempt
deadline** from `ClientOptions`; set it once on the client before you fan out:

```cpp
sugar.service().options().unaryTimeoutMs = 5000;    // ordinary reads (default 30000)
sugar.service().options().tradeTimeoutMs = 60000;   // OrderSend/Modify/Close
sugar.service().options().connectTimeoutMs = 200000;// ConnectEx (a cold terminal start is slow)
sugar.service().options().maxRetries = 5;           // transient-failure / reconnect budget
```

A call that exceeds its deadline is retried up to `maxRetries`, then throws `TransportError`
(`grpcCode == 4`, DEADLINE_EXCEEDED). Timeouts are **per attempt**, not a total budget — size them
for one round-trip, not the whole retry sequence.

## 📚 See also

* [gRPC Streaming](GRPC_STREAM_MANAGEMENT.md) — stream lifecycle & re-subscribing
* [Return Codes & Error Handling](RETURN_CODES_REFERENCE.md) — `ApiError` / `TransportError`
* [Project Map → Transport behaviour](PROJECT_MAP.md) — retry & reconnect shared by every call
