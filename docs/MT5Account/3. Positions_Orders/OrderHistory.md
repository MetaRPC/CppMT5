# ✅ Getting Historical Orders & Deals

> **Request:** historical orders/deals within a time range (`OrdersHistoryData`), with sorting and pagination.

**API Information:**

* **Low-level API:** `MT5Account::orderHistory(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `OrderHistory` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `OrderHistory(OrderHistoryRequest) → OrderHistoryReply`

---

## 💬 Just the essentials

* **What it is.** Returns the order/deal history in a time window, sorted and paginated.
* **Why you need it.** Trade journals, P/L reports, reconciling what actually executed.
* **Sanity check.** `arraytotal()` is the total count across all pages.

---

## 🎯 Purpose

* Build a trade log for a date range.
* Paginate through large histories with `pagenumber` / `itemsperpage` (or `next_page_token`).
* Each `HistoryData` item carries **both** an order view (`history_order`) and a deal view (`history_deal`).

---

## 🧱 Signature

```cpp
// Returns historical orders/deals for the requested window.
// Throws ApiError / TransportError on failure.
pb::OrdersHistoryData orderHistory(const pb::OrderHistoryRequest& req);
```

**Request / Reply:**

```protobuf
OrderHistoryRequest {
  google.protobuf.Timestamp         inputFrom      = 1;   // broker tz
  google.protobuf.Timestamp         inputTo        = 2;
  BMT5_ENUM_ORDER_HISTORY_SORT_TYPE inputSortMode  = 3;
  int32                             pageNumber     = 4;
  int32                             itemsPerPage   = 5;
  google.protobuf.Timestamp         inputFromUtc   = 6;   // UTC equivalents
  google.protobuf.Timestamp         inputToUtc     = 7;
  string                            page_token     = 8;
}

OrderHistoryReply {
  oneof response {
    OrdersHistoryData data  = 1;
    Error             error = 2;
  }
}
```

---

## 🔽 Input

| Field           | C++ setter / type                                   | Description                               |
| --------------- | --------------------------------------------------- | ----------------------------------------- |
| `inputFromUtc`  | `mutable_inputfromutc()->set_seconds(epoch)`        | Window start (UTC). Prefer the `*Utc` fields. |
| `inputToUtc`    | `mutable_inputtoutc()->set_seconds(epoch)`          | Window end (UTC).                         |
| `inputSortMode` | `set_inputsortmode(pb::BMT5_SORT_BY_CLOSE_TIME_DESC)`| Sort order (see enum below).              |
| `pageNumber`    | `set_pagenumber(0)` — int32                         | Zero-based page index.                    |
| `itemsPerPage`  | `set_itemsperpage(100)` — int32                     | Page size.                                |
| `page_token`    | `set_page_token(token)` — string                    | Alternative cursor-based pagination.      |

> `inputFrom` / `inputTo` are in the **broker** time zone; `inputFromUtc` / `inputToUtc` are UTC. Use the UTC pair to avoid timezone surprises.

---

## ⬆️ Output — `OrdersHistoryData`

| Field             | C++ accessor        | Type                | Description                               |
| ----------------- | ------------------- | ------------------- | ----------------------------------------- |
| `HistoryData`     | `history_data()`    | repeated `HistoryData` | The history items on this page.        |
| `ArrayTotal`      | `arraytotal()`      | `int32`             | Total items across all pages.             |
| `PageNumber`      | `pagenumber()`      | `int32`             | Current page index.                       |
| `ItemsPerPage`    | `itemsperpage()`    | `int32`             | Page size echoed back.                    |
| `NextPageToken`   | `next_page_token()` | `string`            | Cursor for the next page (if any).        |

### `HistoryData` — structure

| Accessor          | Type               | Description                            |
| ----------------- | ------------------ | ------------------------------------- |
| `index()`         | `uint32`           | Item index.                           |
| `history_order()` | `OrderHistoryData` | The order as it appears in history.   |
| `history_deal()`  | `DealHistoryData`  | The corresponding executed deal.      |

---

## 🧱 Sort enum — `BMT5_ENUM_ORDER_HISTORY_SORT_TYPE`

| Constant                           | Value | Meaning                     |
| ---------------------------------- | ----- | --------------------------- |
| `BMT5_SORT_BY_OPEN_TIME_ASC`       | 0     | By open time, ascending.    |
| `BMT5_SORT_BY_OPEN_TIME_DESC`      | 1     | By open time, descending.   |
| `BMT5_SORT_BY_CLOSE_TIME_ASC`      | 2     | By close time, ascending.   |
| `BMT5_SORT_BY_CLOSE_TIME_DESC`     | 3     | By close time, descending.  |
| `BMT5_SORT_BY_ORDER_TICKET_ID_ASC` | 4     | By ticket id, ascending.    |
| `BMT5_SORT_BY_ORDER_TICKET_ID_DESC`| 5     | By ticket id, descending.   |

---

## 🧩 Notes & Tips

* **Set the Timestamps via `mutable_*()`.** Protobuf message fields aren't set with a `set_` scalar — use `r.mutable_inputfromutc()->set_seconds(epoch)`.
* **Big histories need a larger inbound limit.** `ClientOptions::maxReceiveMessageBytes` defaults to 64 MB, which already covers large pages.
* **Prefer paging.** Pull in pages (`itemsperpage` ~100–500) rather than an unbounded window.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Last 7 days, newest first, first page

```cpp
#include <ctime>
std::time_t now = std::time(nullptr);

pb::OrderHistoryRequest r;
r.mutable_inputfromutc()->set_seconds(now - 7 * 24 * 3600);
r.mutable_inputtoutc()->set_seconds(now);
r.set_inputsortmode(pb::BMT5_SORT_BY_CLOSE_TIME_DESC);
r.set_pagenumber(0);
r.set_itemsperpage(100);

pb::OrdersHistoryData d = account.orderHistory(r);
std::printf("Total history items: %d (showing %d)\n", d.arraytotal(), d.history_data_size());
for (const auto& h : d.history_data())
    std::printf("  #%u order+deal pair\n", h.index());
```

### 2) Walk all pages

```cpp
int page = 0;
for (;;) {
    pb::OrderHistoryRequest r;
    r.mutable_inputfromutc()->set_seconds(now - 30 * 24 * 3600);
    r.mutable_inputtoutc()->set_seconds(now);
    r.set_inputsortmode(pb::BMT5_SORT_BY_CLOSE_TIME_ASC);
    r.set_pagenumber(page);
    r.set_itemsperpage(200);

    pb::OrdersHistoryData d = account.orderHistory(r);
    if (d.history_data_size() == 0) break;
    // ... process page ...
    if ((page + 1) * d.itemsperpage() >= d.arraytotal()) break;
    ++page;
}
```

---

## 📚 See Also

* [PositionsHistory](./PositionsHistory.md) — closed positions (position-centric history)
* [OpenedOrders](./OpenedOrders.md) — currently open positions and orders
* **Friendly layer:** `MT5Service::orderHistory(...)` returns named `HistoryItem` / `HistoryOrder` / `Deal` structs with UTC times.
