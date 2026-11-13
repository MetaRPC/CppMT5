# ✅ Modifying an Order or Position

> **Request:** change SL/TP (for a position) or price/SL/TP/expiration (for a pending order) — `OrderModifyData`.

**API Information:**

* **Low-level API:** `MT5Account::orderModify(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradingHelper`
* **Proto definition:** `OrderModify` (defined in `mt5-term-api-trading-helper.proto`)

### RPC

* **Service:** `mt5_term_api.TradingHelper`
* **Method:** `OrderModify(OrderModifyRequest) → OrderModifyReply`

---

## 💬 Just the essentials

* **What it is.** Updates an existing ticket: a **position's** SL/TP, or a **pending order's** price, SL/TP, stop-limit and expiration.
* **Why you need it.** Trail a stop, move to break-even, reprice a pending order.
* **Success = `returned_code() == 10009`.**

---

## 🧱 Signature

```cpp
// Modifies the ticket in req. Returns the trade result (check returned_code()).
// Throws ApiError / TransportError on failure.
pb::OrderModifyData orderModify(const pb::OrderModifyRequest& req);
```

**Request / Reply:**

```protobuf
OrderModifyRequest {
  uint64                    ticket               = 1;
  double                    price                = 2;   // pending orders only
  double                    stop_loss            = 3;
  double                    take_profit          = 4;
  double                    stop_limit           = 5;   // *_STOP_LIMIT only
  TMT5_ENUM_ORDER_TYPE_TIME expiration_time_type = 6;
  google.protobuf.Timestamp expiration_time      = 7;
  google.protobuf.Timestamp expiration_time_utc  = 8;
}

OrderModifyReply { oneof response { OrderModifyData data = 1; Error error = 2; } }
```

---

## 🔽 Input

| Field               | C++ setter / type                       | Description                                              |
| ------------------- | --------------------------------------- | ------------------------------------------------------- |
| `ticket`            | `set_ticket(123456)` — uint64           | Position or pending-order ticket to modify.             |
| `price`             | `set_price(1.0820)`                     | New trigger price (**pending orders only**; ignore for positions). |
| `stop_loss`         | `set_stop_loss(1.0800)`                 | New SL (0 = remove).                                    |
| `take_profit`       | `set_take_profit(1.0900)`               | New TP (0 = remove).                                    |
| `stop_limit`        | `set_stop_limit(...)`                   | New stop-limit price (for *_STOP_LIMIT).                |
| `expiration_time_type` | `set_expiration_time_type(...)`      | New expiration policy (pending orders).                 |

---

## ⬆️ Output — `OrderModifyData`

Same shape as [OrderSend](./OrderSend.md)'s result:

| Accessor                     | Type       | Description                                |
| ---------------------------- | ---------- | ------------------------------------------ |
| `returned_code()`            | `uint32_t` | **`10009` = DONE.**                        |
| `returned_code_description()`| `string`   | Human-readable retcode.                     |
| `returned_string_code()`     | `string`   | String retcode.                             |
| `order()` / `deal()`         | `uint64_t` | Affected tickets.                           |
| `price()` / `volume()`       | `double`   | Resulting price/volume.                     |
| `bid()` / `ask()`            | `double`   | Market at execution.                        |
| `comment()`                  | `string`   | Broker comment.                             |

---

## 🧩 Notes & Tips

* **Positions: SL/TP only.** For an open position, set `stop_loss` / `take_profit`; leave `price` as-is (price can't move an open position).
* **Respect stops level.** SL/TP must be at least `SYMBOL_TRADE_STOPS_LEVEL` points away from price, or the modify is rejected.
* **Read current values first** with [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) so you don't accidentally clear an SL/TP you meant to keep.
* **Trade timeout** `ClientOptions::tradeTimeoutMs` (default `60000` ms).

---

## 🔗 Usage Examples

### 1) Move a position's stop to break-even

```cpp
// Find the position
pb::OpenedOrdersRequest or_;
for (const auto& p : account.openedOrders(or_).position_infos()) {
    if (p.symbol() != "EURUSD") continue;
    pb::OrderModifyRequest r;
    r.set_ticket(p.ticket());
    r.set_stop_loss(p.price_open());      // break-even
    r.set_take_profit(p.take_profit());   // keep existing TP
    pb::OrderModifyData res = account.orderModify(r);
    std::printf("Modify retcode: %u\n", res.returned_code());
    break;
}
```

### 2) Reprice a pending order

```cpp
pb::OrderModifyRequest r;
r.set_ticket(987654);
r.set_price(1.0790);
r.set_take_profit(1.0890);
std::printf("retcode %u\n", account.orderModify(r).returned_code());
```

---

## 🧱 Enums used

| Field (input) | Enum | Values |
|---------------|------|--------|
| `expiration_time_type` | `TMT5_ENUM_ORDER_TYPE_TIME` | `TMT5_ORDER_TIME_GTC`(0), `_DAY`(1), `_SPECIFIED`(2), `_SPECIFIED_DAY`(3) |

The output `returned_code` is a trade retcode (`10009` = DONE), not an enum. Full catalog: [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md) · [Return Codes](../../All_Guides/RETURN_CODES_REFERENCE.md).

## 📚 See Also

* [OrderSend](./OrderSend.md) — open/place orders
* [OrderClose](./OrderClose.md) — close a position
* [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) — read the current ticket/SL/TP first
* **Friendly layer:** `MT5Sugar::setStopLoss(ticket, price)` / `setTakeProfit(ticket, price)`.
