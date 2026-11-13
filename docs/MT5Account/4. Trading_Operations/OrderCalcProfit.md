# ✅ Calculating Potential Profit

> **Request:** the profit a trade would yield for a given price move (`OrderCalcProfitData`).

**API Information:**

* **Low-level API:** `MT5Account::orderCalcProfit(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradeFunctions`
* **Proto definition:** `OrderCalcProfit` (defined in `mt5-term-api-trade-functions.proto`)

### RPC

* **Service:** `mt5_term_api.TradeFunctions`
* **Method:** `OrderCalcProfit(OrderCalcProfitRequest) → OrderCalcProfitReply`

---

## 💬 Just the essentials

* **What it is.** Computes the profit (account currency) of moving `volume` of a symbol from `open_price` to `close_price`.
* **Why you need it.** Project TP/SL money value, compare setups, size by reward.
* **Sanity check.** For a BUY, `close_price > open_price` → positive profit.

---

## 🧱 Signature

```cpp
// Returns the profit for the described move. Throws ApiError / TransportError.
pb::OrderCalcProfitData orderCalcProfit(const pb::OrderCalcProfitRequest& req);
```

**Request / Reply:**

```protobuf
OrderCalcProfitRequest {
  ENUM_ORDER_TYPE_TF order_type  = 1;   // BUY/SELL
  string             symbol      = 2;
  double             volume      = 3;
  double             open_price  = 4;
  double             close_price = 5;
}

OrderCalcProfitReply { oneof response { OrderCalcProfitData data = 1; Error error = 2; } }
```

---

## 🔽 Input

| Field         | C++ setter / type                         | Description                 |
| ------------- | ----------------------------------------- | --------------------------- |
| `order_type`  | `set_order_type(pb::ORDER_TYPE_TF_BUY)`   | Direction.                  |
| `symbol`      | `set_symbol("EURUSD")`                    | Instrument.                 |
| `volume`      | `set_volume(1.0)`                         | Lots.                       |
| `open_price`  | `set_open_price(1.0850)`                  | Entry price.                |
| `close_price` | `set_close_price(1.0900)`                 | Exit price.                 |

---

## ⬆️ Output — `OrderCalcProfitData`

| Field    | C++ accessor | C++ type | Description                               |
| -------- | ------------ | -------- | ----------------------------------------- |
| `Profit` | `profit()`   | `double` | Profit in the account currency (can be negative). |

---

## 🧩 Notes & Tips

* **Direction matters.** Use `ORDER_TYPE_TF_BUY` vs `ORDER_TYPE_TF_SELL` — a SELL profits when price falls.
* **Money value of a TP/SL.** Pass your target/stop as `close_price` to see the $ outcome before trading.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Profit of a 50-pip TP on 1 lot

```cpp
pb::OrderCalcProfitRequest r;
r.set_order_type(pb::ORDER_TYPE_TF_BUY);
r.set_symbol("EURUSD");
r.set_volume(1.0);
r.set_open_price(1.0850);
r.set_close_price(1.0900);
std::printf("Profit at TP: %.2f\n", account.orderCalcProfit(r).profit());
```

### 2) Risk/reward of a setup

```cpp
auto calc = [&](double open, double close) {
    pb::OrderCalcProfitRequest r;
    r.set_order_type(pb::ORDER_TYPE_TF_BUY); r.set_symbol("EURUSD");
    r.set_volume(0.10); r.set_open_price(open); r.set_close_price(close);
    return account.orderCalcProfit(r).profit();
};
double reward = calc(1.0850, 1.0900);   // +50 pips
double risk   = calc(1.0850, 1.0820);   // -30 pips
std::printf("R:R = %.2f : %.2f\n", reward, -risk);
```

---

## 📚 See Also

* [OrderCalcMargin](./OrderCalcMargin.md) — margin required
* [OrderCheck](./OrderCheck.md) — full pre-trade validation
* [OrderSend](./OrderSend.md) — place the trade
* **Friendly layer:** `MT5Service::orderCalcProfit(...)`.
