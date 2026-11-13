# ✅ Placing an Order (Market or Pending)

> **Request:** open a market position or place a pending order (`OrderSendData`) — the core trading call.

**API Information:**

* **Low-level API:** `MT5Account::orderSend(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradingHelper`
* **Proto definition:** `OrderSend` (defined in `mt5-term-api-trading-helper.proto`)

### RPC

* **Service:** `mt5_term_api.TradingHelper`
* **Method:** `OrderSend(OrderSendRequest) → OrderSendReply`

---

## 💬 Just the essentials

* **What it is.** Sends a trade request: market BUY/SELL, or a pending LIMIT/STOP/STOP_LIMIT order.
* **Why you need it.** It's how you actually enter the market.
* **Success = `returned_code() == 10009`** (DONE). Always check it — a reply is not the same as a fill.

---

## 🎯 Purpose

* Open a market position (`TMT5_ORDER_TYPE_BUY` / `..._SELL`).
* Place a pending order (`..._BUY_LIMIT`, `..._SELL_STOP`, …) at a chosen `price`.
* Attach SL/TP at entry, set a magic number and comment.

---

## 🧱 Signature

```cpp
// Sends an order. Returns the server's trade result (check returned_code()).
// Throws ApiError / TransportError on transport/validation failure.
pb::OrderSendData orderSend(const pb::OrderSendRequest& req);
```

**Request / Reply:**

```protobuf
OrderSendRequest {
  string               symbol               = 1;
  TMT5_ENUM_ORDER_TYPE operation            = 2;   // BUY/SELL/BUY_LIMIT/...
  double               volume               = 3;   // lots
  double               price                = 4;   // entry price (0 ⇒ market for BUY/SELL)
  double               stop_limit_price     = 5;   // for *_STOP_LIMIT
  double               stop_loss            = 6;
  double               take_profit          = 7;
  uint64               slippage             = 8;   // max deviation (points)
  uint64               expert_id            = 9;   // magic number
  string               comment              = 10;
  TMT5_ENUM_ORDER_TYPE_TIME expiration_time_type = 11;
  google.protobuf.Timestamp expiration_time      = 12;
  google.protobuf.Timestamp expiration_time_utc  = 13;
  int32                quotes_wait_ms       = 14;
}

OrderSendReply { oneof response { OrderSendData data = 1; Error error = 2; } }
```

---

## 🔽 Input (common fields)

| Field                  | C++ setter / type                                  | Description                                     |
| ---------------------- | -------------------------------------------------- | ----------------------------------------------- |
| `symbol`               | `set_symbol("EURUSD")`                             | Instrument (must be selected in Market Watch).  |
| `operation`            | `set_operation(pb::TMT5_ORDER_TYPE_BUY)`           | Order type (see enum).                           |
| `volume`               | `set_volume(0.01)`                                 | Lots (respect `SYMBOL_VOLUME_MIN/STEP`).        |
| `price`                | `set_price(1.0850)`                                | Entry price; for market BUY/SELL pass the current ask/bid (or `0`). Required for pending orders. |
| `stop_limit_price`     | `set_stop_limit_price(...)`                        | Only for `*_STOP_LIMIT` orders.                 |
| `stop_loss`            | `set_stop_loss(1.0800)`                            | SL price (0 = none).                            |
| `take_profit`          | `set_take_profit(1.0900)`                          | TP price (0 = none).                            |
| `slippage`             | `set_slippage(10)`                                 | Max price deviation in points (market orders).  |
| `expert_id`            | `set_expert_id(123456)`                            | Magic number (tag your EA's orders).            |
| `comment`              | `set_comment("my-strategy")`                       | Free-text comment.                              |
| `expiration_time_type` | `set_expiration_time_type(...)`                    | GTC / DAY / SPECIFIED for pending orders.       |

---

## 🧱 Order-type enum — `TMT5_ENUM_ORDER_TYPE`

| Constant                          | Value | Meaning                       |
| --------------------------------- | ----- | ----------------------------- |
| `TMT5_ORDER_TYPE_BUY`             | 0     | Market buy.                   |
| `TMT5_ORDER_TYPE_SELL`            | 1     | Market sell.                  |
| `TMT5_ORDER_TYPE_BUY_LIMIT`       | 2     | Buy limit (below market).     |
| `TMT5_ORDER_TYPE_SELL_LIMIT`      | 3     | Sell limit (above market).    |
| `TMT5_ORDER_TYPE_BUY_STOP`        | 4     | Buy stop (above market).      |
| `TMT5_ORDER_TYPE_SELL_STOP`       | 5     | Sell stop (below market).     |
| `TMT5_ORDER_TYPE_BUY_STOP_LIMIT`  | 6     | Buy stop-limit.               |
| `TMT5_ORDER_TYPE_SELL_STOP_LIMIT` | 7     | Sell stop-limit.              |
| `TMT5_ORDER_TYPE_CLOSE_BY`        | 8     | Close-by (offset opposite).   |

---

## ⬆️ Output — `OrderSendData`

| Field                      | C++ accessor                 | C++ type   | Description                                       |
| -------------------------- | ---------------------------- | ---------- | ------------------------------------------------- |
| `ReturnedCode`             | `returned_code()`            | `uint32_t` | **Trade retcode — `10009` = DONE (success).**     |
| `ReturnedCodeDescription`  | `returned_code_description()`| `string`   | Human-readable description of the retcode.         |
| `ReturnedStringCode`       | `returned_string_code()`     | `string`   | String form of the retcode.                        |
| `RetCodeExternal`          | `ret_code_external()`        | `int32_t`  | External system return code.                       |
| `Deal`                     | `deal()`                     | `uint64_t` | Deal ticket (for market execution).                |
| `Order`                    | `order()`                    | `uint64_t` | Order ticket.                                      |
| `Volume`                   | `volume()`                   | `double`   | Executed/accepted volume.                          |
| `Price`                    | `price()`                    | `double`   | Execution price.                                   |
| `Bid`                      | `bid()`                      | `double`   | Current bid at execution.                          |
| `Ask`                      | `ask()`                      | `double`   | Current ask at execution.                          |
| `Comment`                  | `comment()`                  | `string`   | Broker comment.                                    |
| `RequestId`                | `request_id()`               | `uint32_t` | Request correlation id.                            |

---

## 🧩 Notes & Tips

* **Check the retcode, not just the absence of an exception.** `orderSend()` returns normally for a *rejected* trade too — inspect `returned_code()` (`10009` = DONE). See [Return Codes](../../All_Guides/RETURN_CODES_REFERENCE.md).
* **Select the symbol first** ([SymbolSelect](../2.%20Symbol_Information/SymbolSelect.md)) and **normalize volume** against `SYMBOL_VOLUME_MIN/STEP`.
* **Validate before sending** with [OrderCheck](./OrderCheck.md) when you want to know margin impact up front.
* **Trade timeout.** Trading RPCs use `ClientOptions::tradeTimeoutMs` (default `60000` ms).
* **Pending orders need a `price`** (and `stop_limit_price` for *_STOP_LIMIT) and usually an expiration policy.

---

## 🔗 Usage Examples

### 1) Market BUY 0.01 lot with SL/TP

```cpp
// current ask
pb::SymbolInfoTickRequest tr; tr.set_symbol("EURUSD");
double ask = account.symbolInfoTick(tr).ask();

pb::OrderSendRequest r;
r.set_symbol("EURUSD");
r.set_operation(pb::TMT5_ORDER_TYPE_BUY);
r.set_volume(0.01);
r.set_price(ask);
r.set_stop_loss(ask - 0.0050);
r.set_take_profit(ask + 0.0050);
r.set_slippage(10);
r.set_expert_id(777);
r.set_comment("demo-buy");

pb::OrderSendData res = account.orderSend(r);
if (res.returned_code() == 10009)
    std::printf("DONE: order #%llu @ %.5f\n", (unsigned long long)res.order(), res.price());
else
    std::printf("Rejected: %u %s\n", res.returned_code(), res.returned_code_description().c_str());
```

### 2) Pending BUY LIMIT below market

```cpp
pb::OrderSendRequest r;
r.set_symbol("EURUSD");
r.set_operation(pb::TMT5_ORDER_TYPE_BUY_LIMIT);
r.set_volume(0.01);
r.set_price(1.0800);          // trigger price
r.set_take_profit(1.0900);
pb::OrderSendData res = account.orderSend(r);
std::printf("Pending placed: retcode %u, order #%llu\n",
            res.returned_code(), (unsigned long long)res.order());
```

### 3) A guard helper

```cpp
bool sendOk(MT5Account& a, const pb::OrderSendRequest& r, pb::OrderSendData& out) {
    try {
        out = a.orderSend(r);
        return out.returned_code() == 10009;
    } catch (const ApiError& e)      { std::printf("API: %s\n", e.what()); }
      catch (const TransportError& e){ std::printf("NET: %s\n", e.what()); }
    return false;
}
```

---

## 📚 See Also

* [OrderModify](./OrderModify.md) — change SL/TP or a pending order's price
* [OrderClose](./OrderClose.md) — close a position (full or partial)
* [OrderCheck](./OrderCheck.md) — validate a request & preview margin before sending
* [OrderCalcMargin](./OrderCalcMargin.md) — margin needed for a prospective trade
* [Return Codes](../../All_Guides/RETURN_CODES_REFERENCE.md) — what `returned_code()` means
* **Friendly layer:** `MT5Sugar::buyMarket("EURUSD", 0.01)` / `buyLimitWithSLTP(...)` one-liners.
