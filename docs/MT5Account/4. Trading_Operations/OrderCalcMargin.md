# ✅ Calculating Required Margin

> **Request:** the margin a prospective trade would require (`OrderCalcMarginData`) — before you send it.

**API Information:**

* **Low-level API:** `MT5Account::orderCalcMargin(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradeFunctions`
* **Proto definition:** `OrderCalcMargin` (defined in `mt5-term-api-trade-functions.proto`)

### RPC

* **Service:** `mt5_term_api.TradeFunctions`
* **Method:** `OrderCalcMargin(OrderCalcMarginRequest) → OrderCalcMarginReply`

---

## 💬 Just the essentials

* **What it is.** Computes the margin needed to open a given volume of a symbol at a price.
* **Why you need it.** Pre-trade risk checks and position sizing — "can I afford this?".
* **Sanity check.** Larger volume → larger margin.

---

## 🧱 Signature

```cpp
// Returns the margin required for the described trade. Throws ApiError / TransportError.
pb::OrderCalcMarginData orderCalcMargin(const pb::OrderCalcMarginRequest& req);
```

**Request / Reply:**

```protobuf
OrderCalcMarginRequest {
  ENUM_ORDER_TYPE_TF order_type = 1;   // BUY/SELL/...
  string             symbol     = 2;
  double             volume     = 3;   // lots
  double             open_price = 4;
}

OrderCalcMarginReply { oneof response { OrderCalcMarginData data = 1; Error error = 2; } }
```

---

## 🔽 Input

| Field        | C++ setter / type                              | Description                      |
| ------------ | ---------------------------------------------- | ------------------------------- |
| `order_type` | `set_order_type(pb::ORDER_TYPE_TF_BUY)`        | Direction (see `ENUM_ORDER_TYPE_TF`). |
| `symbol`     | `set_symbol("EURUSD")`                         | Instrument.                     |
| `volume`     | `set_volume(1.0)`                              | Lots.                           |
| `open_price` | `set_open_price(1.0850)`                       | Hypothetical entry price.       |

> `ENUM_ORDER_TYPE_TF` mirrors `TMT5_ENUM_ORDER_TYPE`: `ORDER_TYPE_TF_BUY`(0), `..._SELL`(1), `..._BUY_LIMIT`(2), … `..._CLOSE_BY`(8).

---

## ⬆️ Output — `OrderCalcMarginData`

| Field    | C++ accessor | C++ type | Description                               |
| -------- | ------------ | -------- | ----------------------------------------- |
| `Margin` | `margin()`   | `double` | Required margin in the account currency.  |

---

## 🧩 Notes & Tips

* **Compare with free margin** from [AccountSummary](../1.%20Account_Information/AccountSummary.md) (`account_margin_free()`): trade is affordable if `margin() ≤ free margin` (with a buffer).
* **Great for sizing.** Back out the max volume for a given margin budget by scaling linearly.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Margin for 1 lot EURUSD

```cpp
pb::OrderCalcMarginRequest r;
r.set_order_type(pb::ORDER_TYPE_TF_BUY);
r.set_symbol("EURUSD");
r.set_volume(1.0);
r.set_open_price(1.0850);
std::printf("Margin for 1.0 lot: %.2f\n", account.orderCalcMargin(r).margin());
```

### 2) Affordability check

```cpp
double freeMargin = account.accountSummary().account_margin_free();
pb::OrderCalcMarginRequest r;
r.set_order_type(pb::ORDER_TYPE_TF_BUY); r.set_symbol("EURUSD");
r.set_volume(0.5); r.set_open_price(1.0850);

double need = account.orderCalcMargin(r).margin();
std::printf("%s (need %.2f / free %.2f)\n",
            need <= freeMargin ? "OK" : "NOT ENOUGH MARGIN", need, freeMargin);
```

---

## 📚 See Also

* [OrderCalcProfit](./OrderCalcProfit.md) — expected profit for a move
* [OrderCheck](./OrderCheck.md) — full pre-trade validation (margin + free margin + retcode)
* [OrderSend](./OrderSend.md) — place the trade
* **Friendly layer:** `MT5Service::orderCalcMargin(...)` / `MT5Sugar` risk-sizing helpers.
