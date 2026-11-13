# ✅ Getting Margin Rates for a Symbol

> **Request:** the initial and maintenance margin rates for a symbol and order type (`SymbolInfoMarginRateData`).

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoMarginRate(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoMarginRate` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoMarginRate(SymbolInfoMarginRateRequest) → SymbolInfoMarginRateReply`

---

## 💬 Just the essentials

* **What it is.** Returns the margin rate multipliers for a symbol, per order direction.
* **Why you need it.** Fine-grained margin modeling (some symbols/directions have different rates).
* **Sanity check.** Rates are usually `1.0` for standard FX; they can differ for exotic/leveraged instruments.

---

## 🧱 Signature

```cpp
// Returns margin rates for req.symbol() and req.order_type().
// Throws ApiError / TransportError on failure.
pb::SymbolInfoMarginRateData symbolInfoMarginRate(const pb::SymbolInfoMarginRateRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoMarginRateRequest {
  string           symbol     = 1;
  ENUM_ORDER_TYPE  order_type = 2;   // BUY/SELL/...
}

SymbolInfoMarginRateReply {
  oneof response {
    SymbolInfoMarginRateData data  = 1;   // { double initial_margin_rate; double maintenance_margin_rate; }
    Error                    error = 2;
  }
}
```

---

## 🔽 Input

| Field        | C++ setter / type                          | Description                 |
| ------------ | ------------------------------------------ | --------------------------- |
| `symbol`     | `set_symbol("EURUSD")`                     | Instrument.                 |
| `order_type` | `set_order_type(pb::ORDER_TYPE_BUY)` — `ENUM_ORDER_TYPE` | Direction/type (`ORDER_TYPE_BUY`(0) … `ORDER_TYPE_CLOSE_BY`(8)). |

---

## ⬆️ Output — `SymbolInfoMarginRateData`

| Field                    | C++ accessor                | C++ type | Description                    |
| ------------------------ | --------------------------- | -------- | ------------------------------ |
| `InitialMarginRate`      | `initial_margin_rate()`     | `double` | Initial margin rate multiplier. |
| `MaintenanceMarginRate`  | `maintenance_margin_rate()` | `double` | Maintenance margin rate multiplier. |

---

## 🧩 Notes & Tips

* **Multipliers, not money.** These scale the base margin requirement; combine with [OrderCalcMargin](../4.%20Trading_Operations/OrderCalcMargin.md) for the actual amount.
* **Direction matters.** Query BUY and SELL separately if your broker differentiates.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::SymbolInfoMarginRateRequest r;
r.set_symbol("EURUSD");
r.set_order_type(pb::ORDER_TYPE_BUY);
pb::SymbolInfoMarginRateData d = account.symbolInfoMarginRate(r);
std::printf("initial=%.3f maintenance=%.3f\n",
            d.initial_margin_rate(), d.maintenance_margin_rate());
```

---

## 📚 See Also

* [OrderCalcMargin](../4.%20Trading_Operations/OrderCalcMargin.md) — actual margin for a trade
* [AccountSummary](../1.%20Account_Information/AccountSummary.md) — account margin mode & free margin
* **Friendly layer:** `MT5Service::symbolMarginRate(...)` returns a named `MarginRate` struct.
