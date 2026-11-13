# ✅ Getting Open Positions and Pending Orders

> **Request:** full details of every open **position** and pending **order** (`OpenedOrdersData`) in one call.

**API Information:**

* **Low-level API:** `MT5Account::openedOrders(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `OpenedOrders` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `OpenedOrders(OpenedOrdersRequest) → OpenedOrdersReply`

---

## 💬 Just the essentials

* **What it is.** Returns two lists: open **positions** (`position_infos`) and pending **orders** (`opened_orders`).
* **Why you need it.** The core "what do I have live right now?" call — P/L, SL/TP, volumes, tickets.
* **Sanity check.** `position_infos_size()` matches [PositionsTotal](./PositionsTotal.md).

---

## 🎯 Purpose

* Dashboard of all live exposure (positions) and resting orders.
* Compute total floating P/L, net volume, SL/TP coverage.
* Find a ticket to pass to [OrderModify](../4.%20Trading_Operations/OrderModify.md) / [OrderClose](../4.%20Trading_Operations/OrderClose.md).

---

## 🧱 Signature

```cpp
// Returns all open positions and pending orders. Throws ApiError / TransportError.
pb::OpenedOrdersData openedOrders(const pb::OpenedOrdersRequest& req);
```

**Request / Reply:**

```protobuf
OpenedOrdersRequest {
  BMT5_ENUM_OPENED_ORDER_SORT_TYPE inputSortMode = 1;   // sort order
}

OpenedOrdersReply {
  oneof response {
    OpenedOrdersData data  = 1;
    Error            error = 2;
  }
}
```

---

## 🔽 Input

| Field           | C++ setter / type                       | Description                        |
| --------------- | --------------------------------------- | ---------------------------------- |
| `inputSortMode` | `set_inputsortmode(0)` — sort enum      | Sort order for the returned lists (default `0`). |

```cpp
pb::OpenedOrdersRequest r;
r.set_inputsortmode(0);
pb::OpenedOrdersData d = account.openedOrders(r);
```

---

## ⬆️ Output — `OpenedOrdersData`

| Field            | C++ accessor       | Element type       | Description                       |
| ---------------- | ------------------ | ------------------ | --------------------------------- |
| `PositionInfos`  | `position_infos()` | `PositionInfo`     | Open positions (live exposure).   |
| `OpenedOrders`   | `opened_orders()`  | `OpenedOrderInfo`  | Pending orders (not yet filled).  |

Use `position_infos_size()` / `opened_orders_size()`, `position_infos(i)` / `opened_orders(i)`, or range-for.

### `PositionInfo` — key fields

| Accessor                | Type     | Description                                        |
| ----------------------- | -------- | -------------------------------------------------- |
| `ticket()`              | `int64`  | Position ticket.                                   |
| `symbol()`              | `string` | Symbol.                                            |
| `type()`                | enum     | `TMT5_ENUM_POSITION_TYPE` — 0 = BUY, 1 = SELL.     |
| `volume()`              | `double` | Position volume (lots).                            |
| `price_open()`          | `double` | Open price.                                        |
| `price_current()`       | `double` | Current price.                                     |
| `stop_loss()`           | `double` | SL price (0 = none).                               |
| `take_profit()`         | `double` | TP price (0 = none).                               |
| `profit()`              | `double` | Floating profit in account currency.               |
| `swap()`                | `double` | Accrued swap.                                      |
| `position_commission()` | `double` | Commission.                                        |
| `magic_number()`        | `int64`  | Magic (EA identifier).                             |
| `comment()`             | `string` | Order comment.                                     |
| `open_time()`           | `Timestamp` | Open time (`.seconds()` for epoch).             |
| `identifier()`          | `int64`  | Position identifier (stable across modifications). |
| `reason()`              | enum     | Reason the position was opened.                    |

### `OpenedOrderInfo` — key fields (pending orders)

| Accessor            | Type     | Description                                     |
| ------------------- | -------- | ---------------------------------------------- |
| `ticket()`          | `int64`  | Order ticket.                                  |
| `symbol()`          | `string` | Symbol.                                        |
| `type()`            | enum     | Order type (BUY_LIMIT, SELL_STOP, …).          |
| `state()`           | enum     | Order state.                                   |
| `volume_initial()`  | `double` | Requested volume.                              |
| `volume_current()`  | `double` | Remaining volume.                              |
| `price_open()`      | `double` | Trigger price.                                 |
| `price_current()`   | `double` | Current market price.                          |
| `stop_loss()`       | `double` | SL price.                                      |
| `take_profit()`     | `double` | TP price.                                      |
| `stop_limit()`      | `double` | Stop-limit price (for *_STOP_LIMIT orders).    |
| `magic_number()`    | `int64`  | Magic.                                         |
| `comment()`         | `string` | Comment.                                       |
| `time_setup()`      | `Timestamp` | When the order was placed.                  |
| `time_expiration()` | `Timestamp` | Expiry (0 = GTC).                           |
| `position_id()`     | `int64`  | Linked position id.                            |

---

## 🧩 Notes & Tips

* **Two separate lists.** `position_infos` are *live positions*; `opened_orders` are *pending orders*. Don't mix them.
* **Position type is an enum.** `type()` is `0` (BUY) or `1` (SELL) — not the order-type enum used when placing orders.
* **Guard with [PositionsTotal](./PositionsTotal.md)** if you only need a count.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) List positions with P/L

```cpp
pb::OpenedOrdersRequest r; r.set_inputsortmode(0);
pb::OpenedOrdersData d = account.openedOrders(r);

double total = 0;
for (const auto& p : d.position_infos()) {
    std::printf("#%lld %s %s %.2f @ %.5f  P/L %.2f\n",
                (long long)p.ticket(), p.symbol().c_str(),
                p.type() == 0 ? "BUY" : "SELL",
                p.volume(), p.price_open(), p.profit());
    total += p.profit();
}
std::printf("Total floating P/L: %.2f\n", total);
```

### 2) List pending orders

```cpp
pb::OpenedOrdersRequest r;
pb::OpenedOrdersData d = account.openedOrders(r);
for (const auto& o : d.opened_orders())
    std::printf("Pending #%lld %s vol %.2f @ %.5f\n",
                (long long)o.ticket(), o.symbol().c_str(),
                o.volume_current(), o.price_open());
```

### 3) Find a ticket for a symbol, then close it

```cpp
pb::OpenedOrdersRequest r;
for (const auto& p : account.openedOrders(r).position_infos()) {
    if (p.symbol() == "EURUSD") {
        pb::OrderCloseRequest cr; cr.set_ticket(p.ticket());
        account.orderClose(cr);
        break;
    }
}
```

---

## 🧱 Enums used (in the returned data)

These appear as `int`-valued enum fields on the nested structs (namespace-scope values):

| On | Field | Enum | Values |
|----|-------|------|--------|
| `PositionInfo` | `type()` | `BMT5_ENUM_POSITION_TYPE` | `BMT5_POSITION_TYPE_BUY`(0), `_SELL`(1) |
| `PositionInfo` | `reason()` | `BMT5_ENUM_POSITION_REASON` | `_CLIENT`(0), `_MOBILE`(1), `_WEB`(2), `_EXPERT`(3), `ORDER_REASON_SL`(4), `ORDER_REASON_TP`(5), `ORDER_REASON_SO`(6) |
| `OpenedOrderInfo` | `type()` | `TMT5_ENUM_ORDER_TYPE` | `TMT5_ORDER_TYPE_BUY`(0) … `_CLOSE_BY`(8) |
| `OpenedOrderInfo` | `state()` | `BMT5_ENUM_ORDER_STATE` | `_STARTED`(0), `_PLACED`(1), `_CANCELED`(2), `_PARTIAL`(3), `_FILLED`(4), `_REJECTED`(5), `_EXPIRED`(6), … |

Full catalog: [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md).

## 📚 See Also

* [PositionsTotal](./PositionsTotal.md) — just the count
* [OpenedOrdersTickets](./OpenedOrdersTickets.md) — ticket numbers only (lightweight)
* [OrderModify](../4.%20Trading_Operations/OrderModify.md) / [OrderClose](../4.%20Trading_Operations/OrderClose.md) — act on a ticket
* **Friendly layer:** `MT5Service::openedOrders()` returns named `Position` / `PendingOrder` structs.
