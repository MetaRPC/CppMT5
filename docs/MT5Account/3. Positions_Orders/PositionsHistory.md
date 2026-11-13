# ✅ Getting Closed Positions (Position History)

> **Request:** closed positions within a time range (`PositionsHistoryData`), with sorting and pagination — a position-centric view of history.

**API Information:**

* **Low-level API:** `MT5Account::positionsHistory(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `PositionsHistory` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `PositionsHistory(PositionsHistoryRequest) → PositionsHistoryReply`

---

## 💬 Just the essentials

* **What it is.** Returns **closed positions** in a time window — one row per position (vs [OrderHistory](./OrderHistory.md), which is order/deal-centric).
* **Why you need it.** Per-position P/L reports, win-rate stats, equity curves.
* **Sanity check.** `total_count()` is the total across all pages.

---

## 🧱 Signature

```cpp
// Returns closed positions for the requested window.
// Throws ApiError / TransportError on failure.
pb::PositionsHistoryData positionsHistory(const pb::PositionsHistoryRequest& req);
```

**Request / Reply:**

```protobuf
PositionsHistoryRequest {
  AH_ENUM_POSITIONS_HISTORY_SORT_TYPE sort_type                   = 1;
  google.protobuf.Timestamp           position_open_time_from     = 2;   // broker tz
  google.protobuf.Timestamp           position_open_time_to       = 3;
  int32                               page_number                 = 4;
  int32                               items_per_page              = 5;
  google.protobuf.Timestamp           position_open_time_from_utc = 6;   // UTC equivalents
  google.protobuf.Timestamp           position_open_time_to_utc   = 7;
  string                              page_token                  = 8;
}

PositionsHistoryReply {
  oneof response {
    PositionsHistoryData data  = 1;
    Error                error = 2;
  }
}
```

---

## 🔽 Input

| Field                          | C++ setter / type                                      | Description                       |
| ------------------------------ | ------------------------------------------------------ | --------------------------------- |
| `position_open_time_from_utc`  | `mutable_position_open_time_from_utc()->set_seconds(e)`| Window start (UTC).               |
| `position_open_time_to_utc`    | `mutable_position_open_time_to_utc()->set_seconds(e)`  | Window end (UTC).                 |
| `sort_type`                    | `set_sort_type(pb::AH_POSITION_OPEN_TIME_DESC)`        | Sort order (see enum below).      |
| `page_number`                  | `set_page_number(0)` — int32                           | Zero-based page index.            |
| `items_per_page`               | `set_items_per_page(100)` — int32                      | Page size.                        |
| `page_token`                   | `set_page_token(token)` — string                       | Cursor-based pagination.          |

---

## ⬆️ Output — `PositionsHistoryData`

| Field              | C++ accessor          | Type                         | Description                        |
| ------------------ | --------------------- | ---------------------------- | ---------------------------------- |
| `HistoryPositions` | `history_positions()` | repeated `PositionHistoryInfo` | Closed positions on this page.   |
| `TotalCount`       | `total_count()`       | `int32`                      | Total across all pages.            |
| `NextPageToken`    | `next_page_token()`   | `string`                     | Cursor for the next page (if any). |

### `PositionHistoryInfo` — key fields

| Accessor              | Type     | Description                                     |
| --------------------- | -------- | ---------------------------------------------- |
| `ticket()`            | `int64`  | Position ticket.                               |
| `symbol()`            | `string` | Symbol.                                        |
| `type()`             | enum     | Position type (0 = BUY, 1 = SELL).             |
| `reason()`            | enum     | Why the position closed.                        |
| `volume()`            | `double` | Volume (lots).                                 |
| `price_open()`        | `double` | Open price.                                    |
| `price_current()`     | `double` | Close price.                                   |
| `sl()`                | `double` | Stop loss.                                     |
| `tp()`                | `double` | Take profit.                                   |
| `profit()`            | `double` | Realized profit.                               |
| `swap()`              | `double` | Swap.                                          |
| `magic()`             | `int64`  | Magic (EA id).                                 |
| `from_pending_order()`| `bool`   | Whether it originated from a pending order.    |
| `comment()`           | `string` | Comment.                                       |

---

## 🧱 Sort enum — `AH_ENUM_POSITIONS_HISTORY_SORT_TYPE`

| Constant                      | Value | Meaning                       |
| ----------------------------- | ----- | ----------------------------- |
| `AH_POSITION_OPEN_TIME_ASC`   | 0     | By open time, ascending.      |
| `AH_POSITION_OPEN_TIME_DESC`  | 1     | By open time, descending.     |
| `AH_POSITION_TICKET_ASC`      | 2     | By ticket, ascending.         |
| `AH_POSITION_TICKET_DESC`     | 3     | By ticket, descending.        |

---

## 🧩 Notes & Tips

* **Position-centric.** One row per closed position — simpler than the order/deal pairs in [OrderHistory](./OrderHistory.md).
* **Timestamps via `mutable_*()`.** Use the `*_utc` fields for unambiguous windows.
* **`sl()` / `tp()`.** Note this struct uses the short `sl()`/`tp()` accessors (not `stop_loss()`/`take_profit()`).
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Last 30 days of closed positions, newest first

```cpp
#include <ctime>
std::time_t now = std::time(nullptr);

pb::PositionsHistoryRequest r;
r.mutable_position_open_time_from_utc()->set_seconds(now - 30 * 24 * 3600);
r.mutable_position_open_time_to_utc()->set_seconds(now);
r.set_sort_type(pb::AH_POSITION_OPEN_TIME_DESC);
r.set_page_number(0);
r.set_items_per_page(100);

pb::PositionsHistoryData d = account.positionsHistory(r);
std::printf("Closed positions: %d\n", d.total_count());
for (const auto& p : d.history_positions())
    std::printf("#%lld %s %.2f lots  P/L %.2f\n",
                (long long)p.ticket(), p.symbol().c_str(), p.volume(), p.profit());
```

### 2) Aggregate realized P/L and win rate

```cpp
double gross = 0; int wins = 0, n = 0;
for (const auto& p : account.positionsHistory(r).history_positions()) {
    gross += p.profit(); ++n;
    if (p.profit() > 0) ++wins;
}
std::printf("Net %.2f over %d trades, win rate %.0f%%\n",
            gross, n, n ? 100.0 * wins / n : 0.0);
```

---

## 📚 See Also

* [OrderHistory](./OrderHistory.md) — order/deal-centric history
* [OpenedOrders](./OpenedOrders.md) — currently open positions
* **Friendly layer:** `MT5Service::positionsHistory(...)` returns named `PositionHistory` structs with UTC times.
