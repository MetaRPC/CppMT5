# ✅ Validating a Trade Before Sending

> **Request:** dry-run a trade request and get the resulting margin/equity and a retcode (`OrderCheckData`) — without executing.

**API Information:**

* **Low-level API:** `MT5Account::orderCheck(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradeFunctions`
* **Proto definition:** `OrderCheck` (defined in `mt5-term-api-trade-functions.proto`)

### RPC

* **Service:** `mt5_term_api.TradeFunctions`
* **Method:** `OrderCheck(OrderCheckRequest) → OrderCheckReply`

---

## 💬 Just the essentials

* **What it is.** Validates a full trade request and reports whether it *would* succeed, plus the margin/free-margin/equity after the hypothetical deal.
* **Why you need it.** The safest pre-flight: catches "not enough money", "invalid stops", "invalid volume" before you send.
* **Success = result's `returned_code() == 0`** (check passed). *(This is the check retcode, not the trade `10009`.)*

---

## 🧱 Signature

```cpp
// Validates req.mql_trade_request(). Returns the check result.
// Throws ApiError / TransportError on failure.
pb::OrderCheckData orderCheck(const pb::OrderCheckRequest& req);
```

**Request / Reply:**

```protobuf
OrderCheckRequest { MrpcMqlTradeRequest mql_trade_request = 1; }

OrderCheckReply {
  oneof response {
    OrderCheckData data  = 1;   // { MrpcMqlTradeCheckResult mql_trade_check_result = 1; }
    Error          error = 2;
  }
}
```

---

## 🔽 Input — `MrpcMqlTradeRequest` (key fields)

Build the embedded request via `req.mutable_mql_trade_request()`:

| Field                         | C++ setter / type                                   | Description                      |
| ----------------------------- | --------------------------------------------------- | -------------------------------- |
| `action`                      | `set_action(...)` — `MRPC_ENUM_TRADE_REQUEST_ACTIONS` | DEAL (market) / PENDING / SLTP / MODIFY / REMOVE / CLOSE_BY. |
| `symbol`                      | `set_symbol("EURUSD")`                              | Instrument.                      |
| `order_type`                  | `set_order_type(pb::ORDER_TYPE_TF_BUY)` — `ENUM_ORDER_TYPE_TF` | Direction/type.      |
| `volume`                      | `set_volume(0.10)`                                  | Lots.                            |
| `price`                       | `set_price(1.0850)`                                 | Entry price.                     |
| `stop_loss` / `take_profit`   | `set_stop_loss(...)` / `set_take_profit(...)`       | SL/TP.                           |
| `stop_limit`                  | `set_stop_limit(...)`                               | For *_STOP_LIMIT.                |
| `deviation`                   | `set_deviation(10)`                                 | Max slippage (points).           |
| `expert_advisor_magic_number` | `set_expert_advisor_magic_number(777)`              | Magic number.                    |
| `type_filling` / `type_time`  | `set_type_filling(...)` / `set_type_time(...)`      | Filling & time policy.           |
| `position` / `position_by`    | `set_position(...)` / `set_position_by(...)`        | For modify/close-by.             |
| `comment`                     | `set_comment("...")`                                | Comment.                         |

---

## ⬆️ Output — `OrderCheckData` → `mql_trade_check_result()` (`MrpcMqlTradeCheckResult`)

| Field               | C++ accessor           | C++ type   | Description                                        |
| ------------------- | ---------------------- | ---------- | -------------------------------------------------- |
| `ReturnedCode`      | `returned_code()`      | `uint32_t` | Check retcode (`0` = request is valid).            |
| `Comment`           | `comment()`            | `string`   | Explanation (e.g. reason it would fail).           |
| `Balance`           | `balance_after_deal()` | `double`   | Balance after the hypothetical deal.               |
| `Equity`            | `equity_after_deal()`  | `double`   | Equity after the hypothetical deal.                |
| `Profit`            | `profit()`             | `double`   | Projected profit of the deal.                      |
| `Margin`            | `margin()`             | `double`   | Margin the deal would require.                     |
| `FreeMargin`        | `free_margin()`        | `double`   | Free margin remaining after the deal.              |
| `MarginLevel`       | `margin_level()`       | `double`   | Margin level (%) after the deal.                   |

---

## 🧩 Notes & Tips

* **Two different retcodes.** `orderCheck` result `returned_code() == 0` means *valid*; `orderSend` success is `10009`. Don't confuse them.
* **Set the nested message with `mutable_`.** `req.mutable_mql_trade_request()->set_symbol(...)`.
* **Use before every risky send.** It's the cleanest way to surface "not enough money" / "invalid stops" as data instead of a rejected order.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Validate a market BUY

```cpp
pb::OrderCheckRequest req;
auto* tr = req.mutable_mql_trade_request();
tr->set_action(pb::MRPC_ENUM_TRADE_REQUEST_ACTIONS(0));   // DEAL (market)
tr->set_symbol("EURUSD");
tr->set_order_type(pb::ORDER_TYPE_TF_BUY);
tr->set_volume(0.10);
tr->set_price(1.0850);
tr->set_deviation(10);

pb::OrderCheckData d = account.orderCheck(req);
const auto& res = d.mql_trade_check_result();
if (res.returned_code() == 0)
    std::printf("OK — margin %.2f, free after %.2f\n", res.margin(), res.free_margin());
else
    std::printf("Would fail: %u %s\n", res.returned_code(), res.comment().c_str());
```

### 2) Check then send

```cpp
// (build req as above)
if (account.orderCheck(req).mql_trade_check_result().returned_code() == 0) {
    pb::OrderSendRequest s;
    s.set_symbol("EURUSD"); s.set_operation(pb::TMT5_ORDER_TYPE_BUY);
    s.set_volume(0.10); s.set_price(1.0850); s.set_slippage(10);
    std::printf("send retcode %u\n", account.orderSend(s).returned_code());
}
```

---

## 🧱 Enums used

The embedded `MrpcMqlTradeRequest` is built from several input enums (namespace-scope values, e.g. `pb::TRADE_ACTION_DEAL`):

| Field | Enum | Values |
|-------|------|--------|
| `action` | `MRPC_ENUM_TRADE_REQUEST_ACTIONS` | `TRADE_ACTION_DEAL`(0), `_PENDING`(1), `_SLTP`(2), `_MODIFY`(3), `_REMOVE`(4), `_CLOSE_BY`(5) |
| `order_type` | `ENUM_ORDER_TYPE_TF` | `ORDER_TYPE_TF_BUY`(0) … `ORDER_TYPE_TF_CLOSE_BY`(8) |
| `type_filling` | `MRPC_ENUM_ORDER_TYPE_FILLING` | `ORDER_FILLING_FOK`(0), `_IOC`(1), `_RETURN`(2), `_BOC`(3) |
| `type_time` | `TMT5_ENUM_ORDER_TYPE_TIME` | `TMT5_ORDER_TIME_GTC`(0), `_DAY`(1), `_SPECIFIED`(2), `_SPECIFIED_DAY`(3) |

Full catalog with values: [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md).

## 📚 See Also

* [OrderSend](./OrderSend.md) — execute once the check passes
* [OrderCalcMargin](./OrderCalcMargin.md) / [OrderCalcProfit](./OrderCalcProfit.md) — single-number calculators
* [Return Codes](../../All_Guides/RETURN_CODES_REFERENCE.md)
* **Friendly layer:** `MT5Service::orderCheck(...)` returns a named `OrderCheckResult` struct.
