# MT5Account · Positions & Orders — Overview

> Open positions, pending orders, ticket lists, and historical trades. Use this page to choose the right read API for account state over time.

## 📁 What lives here

### Current state

* **[PositionsTotal](./PositionsTotal.md)** — count of open positions (no args). Service `TradeFunctions`.
* **[OpenedOrders](./OpenedOrders.md)** — full details of all open positions (`position_infos`) and pending orders (`opened_orders`).
* **[OpenedOrdersTickets](./OpenedOrdersTickets.md)** — ticket numbers only (lightweight), positions and pending orders.

### Historical data

* **[OrderHistory](./OrderHistory.md)** — order/deal history in a time range (sorted, paginated). Each item pairs an order view and a deal view.
* **[PositionsHistory](./PositionsHistory.md)** — closed positions in a time range (one row per position; sorted, paginated).

---

## 🧭 Plain English

* "**Do I have anything open?**" → `positionsTotal()`.
* "**Show me everything live**" → `openedOrders()` (positions + pending orders).
* "**Just the tickets**" (cheap, for diffing) → `openedOrdersTickets()`.
* "**What happened last week?**" → `orderHistory()` (orders/deals) or `positionsHistory()` (closed positions).

---

## Quick choose

| If you need…                              | Use                   | Returns                                   | Notes                               |
| ----------------------------------------- | --------------------- | ----------------------------------------- | ----------------------------------- |
| Count of open positions                   | `positionsTotal`      | `int32` (`total_positions()`)             | No arguments.                       |
| All open positions + pending orders       | `openedOrders`        | `PositionInfo[]` + `OpenedOrderInfo[]`    | Sort via `inputSortMode`.           |
| Just ticket numbers                       | `openedOrdersTickets` | `int64[]` positions + `int64[]` orders    | Lightweight; great for change detection. |
| Order/deal history                        | `orderHistory`        | `HistoryData[]` + pagination              | Each item = order view + deal view. |
| Closed positions                          | `positionsHistory`    | `PositionHistoryInfo[]` + pagination      | One row per closed position.        |

---

## ❌ Cross-refs & gotchas

* **Positions vs pending orders.** `openedOrders()` returns them as **two separate lists** — don't conflate `position_infos` with `opened_orders`.
* **Two histories, two shapes.** `orderHistory` is order/deal-centric (each `HistoryData` has `history_order` + `history_deal`); `positionsHistory` is position-centric (one `PositionHistoryInfo` per closed position).
* **Timestamps via `mutable_*()`.** History requests use `google.protobuf.Timestamp` fields — set them with `mutable_...()->set_seconds(epoch)`, and prefer the `*Utc` variants.
* **Accessor naming differs.** Live `PositionInfo` uses `stop_loss()`/`take_profit()`; closed `PositionHistoryInfo` uses `sl()`/`tp()`.
* **Pagination.** History methods support page number + size and a `next_page_token` cursor — don't request unbounded windows.

---

## 🟢 Minimal snippets

```cpp
// Count
std::printf("Open: %d\n", account.positionsTotal().total_positions());
```

```cpp
// Live positions
pb::OpenedOrdersRequest r;
for (const auto& p : account.openedOrders(r).position_infos())
    std::printf("#%lld %s P/L %.2f\n", (long long)p.ticket(), p.symbol().c_str(), p.profit());
```

```cpp
// Tickets only
pb::OpenedOrdersTicketsRequest tr;
pb::OpenedOrdersTicketsData td = account.openedOrdersTickets(tr);
std::printf("%d open position tickets\n", td.opened_position_tickets_size());
```

```cpp
// Closed positions, last 7 days
#include <ctime>
std::time_t now = std::time(nullptr);
pb::PositionsHistoryRequest hr;
hr.mutable_position_open_time_from_utc()->set_seconds(now - 7*24*3600);
hr.mutable_position_open_time_to_utc()->set_seconds(now);
hr.set_sort_type(pb::AH_POSITION_OPEN_TIME_DESC);
std::printf("Closed: %d\n", account.positionsHistory(hr).total_count());
```

---

## See also

* **Trading:** [OrderSend](../4.%20Trading_Operations/OrderSend.md) / [OrderModify](../4.%20Trading_Operations/OrderModify.md) / [OrderClose](../4.%20Trading_Operations/OrderClose.md) — act on tickets found here
* **Account:** [AccountSummary](../1.%20Account_Information/AccountSummary.md) — equity/margin snapshot
* **Streaming:** [OnPositionsAndPendingOrdersTickets](../7.%20Streaming_Methods/OnPositionsAndPendingOrdersTickets.md) — push-based ticket updates
