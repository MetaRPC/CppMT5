# ✅ Streaming Live Ticks

> **Subscribe:** receive a callback for every new tick on one or more symbols (`OnSymbolTickData`), on a background thread.

**API Information:**

* **Low-level API:** `MT5Account::onSymbolTick(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.SubscriptionService`
* **Proto definition:** `OnSymbolTick` (defined in `mt5-term-api-subscriptions.proto`)

### RPC

* **Service:** `mt5_term_api.SubscriptionService`
* **Method:** `OnSymbolTick(OnSymbolTickRequest) → stream OnSymbolTickData`

---

## 💬 Just the essentials

* **What it is.** A **server stream**: the SDK reads ticks on a background thread and calls your callback for each one.
* **Why you need it.** Push-based live prices — no polling loop.
* **Lifecycle.** The call returns a `std::unique_ptr<AccountStream>` handle: **keep it alive** to keep receiving; `stop()` (or destroying it) ends the subscription.

---

## 🧱 Signature

```cpp
// Opens a tick stream for req.symbol_names(). `callback` runs on a background
// reader thread for every OnSymbolTickData. Returns a handle that owns the stream.
// Throws ApiError / TransportError while opening.
std::unique_ptr<AccountStream> onSymbolTick(
    const pb::OnSymbolTickRequest& req,
    std::function<void(const pb::OnSymbolTickData&)> callback);
```

**Request / Stream:**

```protobuf
OnSymbolTickRequest { repeated string symbol_names = 1; }

// server streams OnSymbolTickData:
OnSymbolTickData {
  MrpcSubscriptionMqlTick symbol_tick               = 1;
  string                  terminal_instance_guid_id = 2;
}
```

---

## 🔽 Input

| Field          | C++ setter / type                       | Description                      |
| -------------- | --------------------------------------- | -------------------------------- |
| `symbol_names` | `add_symbol_names("EURUSD")` (repeated) | Symbols to stream (one or many). |

> **Prefer one stream with many symbols** over many single-symbol streams.

---

## ⬆️ Payload — `OnSymbolTickData`

| Accessor                    | Type                      | Description                     |
| --------------------------- | ------------------------- | ------------------------------- |
| `symbol_tick()`             | `MrpcSubscriptionMqlTick` | The tick (see below).           |
| `terminal_instance_guid_id()` | `string`                | Terminal session id.            |

### `MrpcSubscriptionMqlTick`

| Accessor        | Type        | Description                              |
| --------------- | ----------- | ---------------------------------------- |
| `symbol()`      | `string`    | Symbol.                                  |
| `bid()` / `ask()` | `double`  | Quote.                                   |
| `volume()`      | `uint64`    | Volume.                                  |
| `time()`        | `Timestamp` | Tick time (broker tz).                   |
| `time_utc()`    | `Timestamp` | Tick time (UTC).                         |
| `time_msc()` / `time_msc_utc()` | `int64` | Milliseconds (broker / UTC).     |
| `flags()`       | `uint32`    | Tick change flags.                       |

---

## 🧩 Notes & Tips

* **Callback runs on the reader thread.** Don't block it — push slow work to another thread/queue. Guard shared state (`std::atomic`, a mutex).
* **Keep the handle alive.** If the `unique_ptr` is destroyed, the stream stops. Store it for as long as you want ticks.
* **Exceptions in the callback are swallowed** by the reader — handle errors inside it.
* **`AccountStream` handle:** `stop()`, `running()`, `count()` (payloads received), `lastError()`.
* See the [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md) guide.

---

## 🔗 Usage Examples

### 1) Stream two symbols for a while

```cpp
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<int> ticks{0};

pb::OnSymbolTickRequest req;
req.add_symbol_names("EURUSD");
req.add_symbol_names("GBPUSD");

auto stream = account.onSymbolTick(req, [&](const pb::OnSymbolTickData& d) {
    const auto& t = d.symbol_tick();
    std::printf("%s  %.5f / %.5f\n", t.symbol().c_str(), t.bid(), t.ask());
    ++ticks;
});

std::this_thread::sleep_for(std::chrono::seconds(10));
stream->stop();
std::printf("Received %d ticks\n", ticks.load());
```

### 2) Keep only the latest price per symbol

```cpp
#include <map>
#include <mutex>
std::mutex m;
std::map<std::string, double> lastBid;

auto stream = account.onSymbolTick(req, [&](const pb::OnSymbolTickData& d) {
    std::lock_guard<std::mutex> lk(m);
    lastBid[d.symbol_tick().symbol()] = d.symbol_tick().bid();
});
```

---

## 📚 See Also

* [SymbolInfoTick](../2.%20Symbol_Information/SymbolInfoTick.md) — one-shot current tick (no stream)
* [OnTrade](./OnTrade.md) — stream of trade events
* [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md) — lifecycle & threading rules
* **Friendly layer:** `MT5Service::subscribeTicks({"EURUSD"}, cb)` delivers a named `Tick` with UTC time.
