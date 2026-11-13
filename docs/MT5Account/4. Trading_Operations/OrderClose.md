# ✅ Closing a Position (Full or Partial)

> **Request:** close an open position by ticket, fully or partially (`OrderCloseData`).

**API Information:**

* **Low-level API:** `MT5Account::orderClose(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradingHelper`
* **Proto definition:** `OrderClose` (defined in `mt5-term-api-trading-helper.proto`)

### RPC

* **Service:** `mt5_term_api.TradingHelper`
* **Method:** `OrderClose(OrderCloseRequest) → OrderCloseReply`

---

## 💬 Just the essentials

* **What it is.** Closes a position (or part of it); can also delete a pending order by ticket.
* **Why you need it.** Exit the market, take profit/cut loss, scale out.
* **Success = `returned_code() == 10009`.**

---

## 🧱 Signature

```cpp
// Closes the ticket in req. Returns the trade result (check returned_code()).
// Throws ApiError / TransportError on failure.
pb::OrderCloseData orderClose(const pb::OrderCloseRequest& req);
```

**Request / Reply:**

```protobuf
OrderCloseRequest {
  uint64 ticket   = 1;
  double volume   = 2;   // 0 ⇒ close full volume; >0 ⇒ partial close
  int32  slippage = 3;   // max deviation (points)
}

OrderCloseReply { oneof response { OrderCloseData data = 1; Error error = 2; } }
```

---

## 🔽 Input

| Field      | C++ setter / type            | Description                                           |
| ---------- | ---------------------------- | ---------------------------------------------------- |
| `ticket`   | `set_ticket(123456)` — uint64 | Position (or pending-order) ticket to close.         |
| `volume`   | `set_volume(0.0)` — double   | `0` → close the whole position; a smaller value → partial close. |
| `slippage` | `set_slippage(10)` — int32   | Max price deviation (points).                        |

---

## ⬆️ Output — `OrderCloseData`

| Field                      | C++ accessor                  | C++ type                | Description                               |
| -------------------------- | ----------------------------- | ----------------------- | ----------------------------------------- |
| `ReturnedCode`             | `returned_code()`             | `uint32_t`              | **`10009` = DONE.**                       |
| `ReturnedCodeDescription`  | `returned_code_description()` | `string`                | Human-readable retcode.                   |
| `ReturnedStringCode`       | `returned_string_code()`      | `string`                | String retcode.                           |
| `CloseMode`                | `close_mode()`                | `MRPC_ORDER_CLOSE_MODE` | How the close was performed (netting/hedging/close-by). |

---

## 🧩 Notes & Tips

* **Partial close must respect volume step.** The remaining and closed volumes must each be a multiple of `SYMBOL_VOLUME_STEP` and ≥ `SYMBOL_VOLUME_MIN`.
* **Hedging vs netting.** On a netting account closing is really an opposite deal; `close_mode()` tells you what the server did.
* **Check the retcode** — a normal return does not guarantee a fill.
* **Trade timeout** `ClientOptions::tradeTimeoutMs` (default `60000` ms).

---

## 🔗 Usage Examples

### 1) Close a whole position

```cpp
pb::OrderCloseRequest r;
r.set_ticket(123456);
r.set_volume(0.0);        // full
r.set_slippage(10);
pb::OrderCloseData res = account.orderClose(r);
std::printf("Close retcode: %u\n", res.returned_code());
```

### 2) Scale out half

```cpp
pb::OpenedOrdersRequest or_;
for (const auto& p : account.openedOrders(or_).position_infos()) {
    if (p.symbol() != "EURUSD") continue;
    pb::OrderCloseRequest r;
    r.set_ticket(p.ticket());
    r.set_volume(p.volume() / 2.0);   // partial
    r.set_slippage(10);
    std::printf("Partial close retcode: %u\n", account.orderClose(r).returned_code());
    break;
}
```

### 3) Close everything

```cpp
pb::OpenedOrdersRequest or_;
for (const auto& p : account.openedOrders(or_).position_infos()) {
    pb::OrderCloseRequest r; r.set_ticket(p.ticket()); r.set_slippage(10);
    account.orderClose(r);
}
```

---

## 📚 See Also

* [OrderSend](./OrderSend.md) — open positions
* [OrderModify](./OrderModify.md) — change SL/TP instead of closing
* [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) — find the ticket and its volume
* **Friendly layer:** `MT5Sugar::closePosition(ticket)` / `closeAllPositions()`.
