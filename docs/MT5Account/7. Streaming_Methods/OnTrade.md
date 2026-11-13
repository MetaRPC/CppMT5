# ✅ Streaming Trade Events

> **Subscribe:** receive a callback whenever positions/orders change (`OnTradeData`) — opened, closed, modified.

**API Information:**

* **Low-level API:** `MT5Account::onTrade(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.SubscriptionService`
* **Proto definition:** `OnTrade` (defined in `mt5-term-api-subscriptions.proto`)

### RPC

* **Service:** `mt5_term_api.SubscriptionService`
* **Method:** `OnTrade(OnTradeRequest) → stream OnTradeData`

---

## 💬 Just the essentials

* **What it is.** A server stream of **trade events**: new/disappeared positions, orders and history deals.
* **Why you need it.** React immediately to fills, closes and modifications instead of polling [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md).
* **Lifecycle.** Returns a `std::unique_ptr<AccountStream>` — keep it alive; `stop()` to end.

---

## 🧱 Signature

```cpp
// Streams trade events to `callback` on a background reader thread.
// Throws ApiError / TransportError while opening.
std::unique_ptr<AccountStream> onTrade(
    const pb::OnTradeRequest& req,
    std::function<void(const pb::OnTradeData&)> callback);
```

**Request / Stream:**

```protobuf
OnTradeRequest {}   // (empty)

// server streams OnTradeData:
OnTradeData {
  MT5_SUB_ENUM_EVENT_GROUP_TYPE type                      = 1;
  OnTadeEventData               event_data                = 2;
  OnEventAccountInfo            account_info              = 3;
  string                        terminal_instance_guid_id = 4;
}
```

---

## 🔽 Input

| Parameter | Type                     | Description               |
| --------- | ------------------------ | ------------------------- |
| `req`     | `const pb::OnTradeRequest&` | Empty request message. |

---

## ⬆️ Payload — `OnTradeData`

| Accessor           | Type                            | Description                                 |
| ------------------ | ------------------------------- | ------------------------------------------- |
| `type()`           | `MT5_SUB_ENUM_EVENT_GROUP_TYPE` | Event group/type.                           |
| `event_data()`     | `OnTadeEventData`               | What changed (see below).                   |
| `account_info()`   | `OnEventAccountInfo`            | Account snapshot at the event.              |
| `terminal_instance_guid_id()` | `string`             | Terminal session id.                        |

### `OnTadeEventData` — "what changed" lists

Each is a repeated collection (`*_size()` + indexed access):

| Accessor                       | Description                          |
| ------------------------------ | ----------------------------------- |
| `new_positions()`              | Positions that appeared.            |
| `disappeared_positions()`      | Positions that closed.              |
| `new_orders()` / `disappeared_orders()` | Pending orders added/removed. |
| `new_history_orders()` / `disappeared_history_orders()` | History order changes. |
| `new_history_deals()` / `disappeared_history_deals()`   | History deal changes.  |

---

## 🧩 Notes & Tips

* **Event-driven, not a full snapshot.** Each message carries *deltas* (what changed). Keep your own state, or re-read [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) when you need the full picture.
* **Callback on the reader thread** — don't block; guard shared state.
* **Keep the handle alive**; `stop()` to end. Handle API: `running()`, `count()`, `lastError()`.
* See [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md).

---

## 🔗 Usage Example

```cpp
#include <thread>
#include <chrono>

pb::OnTradeRequest req;
auto stream = account.onTrade(req, [](const pb::OnTradeData& d) {
    const auto& e = d.event_data();
    if (e.new_positions_size() > 0)
        std::printf("Opened %d position(s)\n", e.new_positions_size());
    if (e.disappeared_positions_size() > 0)
        std::printf("Closed %d position(s)\n", e.disappeared_positions_size());
});

std::this_thread::sleep_for(std::chrono::minutes(1));
stream->stop();
```

---

## 🧱 Enums used (in the returned data)

| Field | Enum | Values |
|-------|------|--------|
| `type()` | `MT5_SUB_ENUM_EVENT_GROUP_TYPE` | `OrderProfit`(0), `OrderUpdate`(1) |

The nested order/position/deal infos also carry their own type/state/entry enums (`BMT5_ENUM_POSITION_TYPE`, `BMT5_ENUM_ORDER_STATE`, `BMT5_ENUM_DEAL_ENTRY_TYPE`, …). Full catalog: [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md).

## 📚 See Also

* [OnTradeTransaction](./OnTradeTransaction.md) — lower-level transaction log (request/result/transaction)
* [OnPositionProfit](./OnPositionProfit.md) — periodic P/L updates
* [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) — full current snapshot
* **Friendly layer:** `MT5Service::streamTradeUpdates(cb)` delivers named `TradeEvent` structs.
