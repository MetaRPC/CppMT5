# ✅ Streaming Position & Pending-Order Ticket Snapshots

> **Subscribe:** receive periodic lists of open position tickets and pending-order tickets (`OnPositionsAndPendingOrdersTicketsData`) on a timer — a lightweight change feed.

**API Information:**

* **Low-level API:** `MT5Account::onPositionsAndPendingOrdersTickets(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.SubscriptionService`
* **Proto definition:** `OnPositionsAndPendingOrdersTickets` (defined in `mt5-term-api-subscriptions.proto`)

### RPC

* **Service:** `mt5_term_api.SubscriptionService`
* **Method:** `OnPositionsAndPendingOrdersTickets(OnPositionsAndPendingOrdersTicketsRequest) → stream OnPositionsAndPendingOrdersTicketsData`

---

## 💬 Just the essentials

* **What it is.** A timer-driven stream of just the **ticket numbers** currently open (positions + pending orders), with a server timestamp.
* **Why you need it.** The cheapest way to notice something opened/closed — diff successive snapshots.
* **Lifecycle.** Returns a `std::unique_ptr<AccountStream>` — keep it alive; `stop()` to end.

---

## 🧱 Signature

```cpp
// Streams periodic ticket snapshots to `callback` on a background thread.
// Throws ApiError / TransportError while opening.
std::unique_ptr<AccountStream> onPositionsAndPendingOrdersTickets(
    const pb::OnPositionsAndPendingOrdersTicketsRequest& req,
    std::function<void(const pb::OnPositionsAndPendingOrdersTicketsData&)> callback);
```

**Request / Stream:**

```protobuf
OnPositionsAndPendingOrdersTicketsRequest {
  int32 timer_period_milliseconds = 1;
}

OnPositionsAndPendingOrdersTicketsData {
  repeated uint64 position_tickets          = 1;
  repeated uint64 pending_order_tickets     = 2;
  google.protobuf.Timestamp server_time     = 3;
  google.protobuf.Timestamp server_time_utc = 4;
  string          terminal_instance_guid_id = 5;
}
```

---

## 🔽 Input

| Field                       | C++ setter / type                     | Description          |
| --------------------------- | ------------------------------------- | -------------------- |
| `timer_period_milliseconds` | `set_timer_period_milliseconds(1000)` | Push interval in ms. |

---

## ⬆️ Payload — `OnPositionsAndPendingOrdersTicketsData`

| Accessor                  | Type              | Description                         |
| ------------------------- | ----------------- | ----------------------------------- |
| `position_tickets()`      | repeated `uint64` | Open position tickets.              |
| `pending_order_tickets()` | repeated `uint64` | Pending order tickets.              |
| `server_time()`           | `Timestamp`       | Snapshot time (broker tz).          |
| `server_time_utc()`       | `Timestamp`       | Snapshot time (UTC).                |
| `terminal_instance_guid_id()` | `string`      | Terminal session id.                |

Use `position_tickets_size()` / `pending_order_tickets_size()` and indexed/range access.

---

## 🧩 Notes & Tips

* **Snapshot, not deltas.** Each message is the *current* full list — compute diffs against the previous one to detect opens/closes.
* **Lightest stream.** Only tickets + timestamp; pair with [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) when a change warrants full detail.
* **Callback on the reader thread** — don't block; guard shared state.
* **Keep the handle alive**; `stop()` to end. Handle API: `running()`, `count()`, `lastError()`.
* See [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md).

---

## 🔗 Usage Example

```cpp
#include <thread>
#include <chrono>
#include <set>

std::set<uint64_t> prev;

pb::OnPositionsAndPendingOrdersTicketsRequest req;
req.set_timer_period_milliseconds(1000);

auto stream = account.onPositionsAndPendingOrdersTickets(req,
    [&](const pb::OnPositionsAndPendingOrdersTicketsData& d) {
        std::set<uint64_t> now(d.position_tickets().begin(), d.position_tickets().end());
        for (uint64_t t : now)  if (!prev.count(t)) std::printf("opened #%llu\n", (unsigned long long)t);
        for (uint64_t t : prev) if (!now.count(t))  std::printf("closed #%llu\n", (unsigned long long)t);
        prev.swap(now);
    });

std::this_thread::sleep_for(std::chrono::seconds(30));
stream->stop();
```

---

## 📚 See Also

* [OnPositionProfit](./OnPositionProfit.md) — periodic P/L (heavier)
* [OpenedOrdersTickets](../3.%20Positions_Orders/OpenedOrdersTickets.md) — one-shot ticket list
* [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md)
* **Friendly layer:** `MT5Service::streamTickets(cb)` delivers a named `TicketsSnapshot` struct.
