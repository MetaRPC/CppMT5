# MT5Account · Trading Operations — Overview

> Order execution, position management, and pre-trade calculations. This is the group that actually moves money — read the notes carefully.

## 📁 What lives here

### Order execution & management (`TradingHelper`)

* **[OrderSend](./OrderSend.md)** — open a market position or place a pending order.
* **[OrderModify](./OrderModify.md)** — change SL/TP (position) or price/SL/TP/expiration (pending).
* **[OrderClose](./OrderClose.md)** — close a position fully or partially.

### Pre-trade calculations (`TradeFunctions`)

* **[OrderCalcMargin](./OrderCalcMargin.md)** — margin required for a prospective trade.
* **[OrderCalcProfit](./OrderCalcProfit.md)** — profit for a given price move.
* **[OrderCheck](./OrderCheck.md)** — full dry-run validation (margin, free margin, retcode).

---

## 🧭 Plain English

* "**Enter the market**" → `orderSend()`.
* "**Change my stop / move my pending**" → `orderModify()`.
* "**Get out**" → `orderClose()`.
* "**Can I afford it? Will it work?**" → `orderCalcMargin()` / `orderCalcProfit()` / `orderCheck()` first.

> **⚠️ Golden rule:** a normal return is **not** a fill. For `orderSend/Modify/Close`, success is **`returned_code() == 10009`** (DONE). For `orderCheck`, a valid request is **check `returned_code() == 0`**.

---

## Quick choose

| If you need…                         | Use               | Returns                         | Success check                 |
| ------------------------------------ | ----------------- | ------------------------------- | ----------------------------- |
| Open / place an order                | `orderSend`       | `OrderSendData`                 | `returned_code() == 10009`    |
| Change SL/TP or pending price        | `orderModify`     | `OrderModifyData`               | `returned_code() == 10009`    |
| Close (full/partial)                 | `orderClose`      | `OrderCloseData`                | `returned_code() == 10009`    |
| Margin for a trade                   | `orderCalcMargin` | `double` (`margin()`)           | —                             |
| Profit for a move                    | `orderCalcProfit` | `double` (`profit()`)           | —                             |
| Validate everything up front         | `orderCheck`      | `MrpcMqlTradeCheckResult`       | result `returned_code() == 0` |

---

## 🧱 Order-type enums

Two parallel enums, 1:1 by value:

| Use in…                              | Enum                   | Values (0…8)                                                     |
| ------------------------------------ | ---------------------- | --------------------------------------------------------------- |
| `orderSend` (`operation`)            | `TMT5_ENUM_ORDER_TYPE` | `TMT5_ORDER_TYPE_BUY`, `..._SELL`, `..._BUY_LIMIT`, `..._SELL_LIMIT`, `..._BUY_STOP`, `..._SELL_STOP`, `..._BUY_STOP_LIMIT`, `..._SELL_STOP_LIMIT`, `..._CLOSE_BY` |
| `orderCalc*` / `orderCheck` (`order_type`) | `ENUM_ORDER_TYPE_TF` | `ORDER_TYPE_TF_BUY`, `..._SELL`, `..._BUY_LIMIT`, … `..._CLOSE_BY` |

`(int)TMT5_ORDER_TYPE_BUY == (int)ORDER_TYPE_TF_BUY == 0`, and so on.

---

## ❌ Cross-refs & gotchas

* **Check the retcode.** Covered above — the single most common mistake.
* **Normalize volume & respect stops level.** Snap volume to `SYMBOL_VOLUME_STEP` (≥ `SYMBOL_VOLUME_MIN`); keep SL/TP ≥ `SYMBOL_TRADE_STOPS_LEVEL` points from price. See [SymbolInfoDouble](../2.%20Symbol_Information/SymbolInfoDouble.md) / [SymbolInfoInteger](../2.%20Symbol_Information/SymbolInfoInteger.md).
* **Select the symbol first.** [SymbolSelect](../2.%20Symbol_Information/SymbolSelect.md) before trading an unusual symbol.
* **Timestamps via `mutable_*()`.** Expiration fields are `google.protobuf.Timestamp`.
* **Trade timeout.** Trading RPCs use `ClientOptions::tradeTimeoutMs` (default `60000` ms), longer than read RPCs.
* **`orderCheck` embeds a `MrpcMqlTradeRequest`** — set it via `req.mutable_mql_trade_request()`.

---

## 🟢 Minimal snippets

```cpp
// Market BUY with SL/TP
pb::OrderSendRequest r;
r.set_symbol("EURUSD"); r.set_operation(pb::TMT5_ORDER_TYPE_BUY);
r.set_volume(0.01); r.set_price(1.0850);
r.set_stop_loss(1.0800); r.set_take_profit(1.0900); r.set_slippage(10);
std::printf("retcode %u\n", account.orderSend(r).returned_code());   // 10009 = DONE
```

```cpp
// Margin for 1 lot
pb::OrderCalcMarginRequest m;
m.set_order_type(pb::ORDER_TYPE_TF_BUY); m.set_symbol("EURUSD");
m.set_volume(1.0); m.set_open_price(1.0850);
std::printf("margin %.2f\n", account.orderCalcMargin(m).margin());
```

```cpp
// Close a ticket
pb::OrderCloseRequest c; c.set_ticket(123456); c.set_slippage(10);
std::printf("close retcode %u\n", account.orderClose(c).returned_code());
```

---

## See also

* **Positions:** [OpenedOrders](../3.%20Positions_Orders/OpenedOrders.md) — find tickets to modify/close
* **Return codes:** [Return Codes Reference](../../All_Guides/RETURN_CODES_REFERENCE.md)
* **Streaming:** [OnTrade](../7.%20Streaming_Methods/OnTrade.md) — react to fills and changes
* **Friendly layers:** `MT5Sugar` one-liners (`buyMarket`, `sellLimitWithSLTP`, `closeAllPositions`) wrap all of this.
