# ✅ Streaming Position Profit Updates

> **Subscribe:** receive periodic profit/loss snapshots for your positions (`OnPositionProfitData`) on a timer.

**API Information:**

* **Low-level API:** `MT5Account::onPositionProfit(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.SubscriptionService`
* **Proto definition:** `OnPositionProfit` (defined in `mt5-term-api-subscriptions.proto`)

### RPC

* **Service:** `mt5_term_api.SubscriptionService`
* **Method:** `OnPositionProfit(OnPositionProfitRequest) → stream OnPositionProfitData`

---

## 💬 Just the essentials

* **What it is.** A timer-driven stream: every `timer_period_milliseconds` the server pushes which positions are new/updated/deleted, with their profit.
* **Why you need it.** Live P/L dashboards and profit-based exits without polling.
* **Lifecycle.** Returns a `std::unique_ptr<AccountStream>` — keep it alive; `stop()` to end.

---

## 🧱 Signature

```cpp
// Streams periodic position-profit updates to `callback` on a background thread.
// Throws ApiError / TransportError while opening.
std::unique_ptr<AccountStream> onPositionProfit(
    const pb::OnPositionProfitRequest& req,
    std::function<void(const pb::OnPositionProfitData&)> callback);
```

**Request / Stream:**

```protobuf
OnPositionProfitRequest {
  int32 timer_period_milliseconds = 1;   // how often to push
  bool  ignore_empty_data         = 2;   // skip ticks with no changes
}

OnPositionProfitData {
  MT5_SUB_ENUM_EVENT_GROUP_TYPE type                      = 1;
  repeated OnPositionProfitPositionInfo new_positions     = 2;
  repeated OnPositionProfitPositionInfo updated_positions = 3;
  repeated OnPositionProfitPositionInfo deleted_positions = 4;
  OnEventAccountInfo            account_info              = 5;
  string                        terminal_instance_guid_id = 6;
}
```

---

## 🔽 Input

| Field                       | C++ setter / type                          | Description                                      |
| --------------------------- | ------------------------------------------ | ------------------------------------------------ |
| `timer_period_milliseconds` | `set_timer_period_milliseconds(1000)`      | Push interval in ms.                             |
| `ignore_empty_data`         | `set_ignore_empty_data(true)`              | If `true`, suppress pushes when nothing changed. |

---

## ⬆️ Payload — `OnPositionProfitData`

| Accessor               | Type                                | Description                        |
| ---------------------- | ----------------------------------- | ---------------------------------- |
| `new_positions()`      | repeated `OnPositionProfitPositionInfo` | Positions that appeared.       |
| `updated_positions()`  | repeated `OnPositionProfitPositionInfo` | Positions whose profit changed.|
| `deleted_positions()`  | repeated `OnPositionProfitPositionInfo` | Positions that closed.         |
| `account_info()`       | `OnEventAccountInfo`                | Account snapshot.                  |
| `terminal_instance_guid_id()` | `string`                     | Terminal session id.               |

### `OnPositionProfitPositionInfo`

| Accessor            | Type     | Description              |
| ------------------- | -------- | ----------------------- |
| `index()`           | `uint`   | Index within the batch. |
| `ticket()`          | `uint64` | Position ticket.        |
| `position_symbol()` | `string` | Symbol.                 |
| `profit()`          | `double` | Current profit.         |

---

## 🧩 Notes & Tips

* **Set a sane interval.** 500–2000 ms is usually plenty; very small periods add load for little benefit.
* **Use `ignore_empty_data`** to avoid empty ticks when nothing changed.
* **Callback on the reader thread** — don't block; guard shared state.
* **Keep the handle alive**; `stop()` to end. Handle API: `running()`, `count()`, `lastError()`.
* See [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md).

---

## 🔗 Usage Example

```cpp
#include <thread>
#include <chrono>

pb::OnPositionProfitRequest req;
req.set_timer_period_milliseconds(1000);
req.set_ignore_empty_data(true);

auto stream = account.onPositionProfit(req, [](const pb::OnPositionProfitData& d) {
    for (const auto& p : d.updated_positions())
        std::printf("#%llu %s profit %.2f\n",
                    (unsigned long long)p.ticket(), p.position_symbol().c_str(), p.profit());
});

std::this_thread::sleep_for(std::chrono::seconds(30));
stream->stop();
```

---

## 🧱 Enums used (in the returned data)

| Field | Enum | Values |
|-------|------|--------|
| `type()` | `MT5_SUB_ENUM_EVENT_GROUP_TYPE` | `OrderProfit`(0), `OrderUpdate`(1) |

Full catalog: [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md).

## 📚 See Also

* [OnPositionsAndPendingOrdersTickets](./OnPositionsAndPendingOrdersTickets.md) — periodic ticket snapshots (lighter)
* [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) — full on-demand snapshot
* [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md)
* **Friendly layer:** `MT5Service::streamPositionProfit(cb)` delivers named `PositionProfitEvent` structs.
