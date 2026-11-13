# MT5Account · Account Information — Overview

> Account balance, equity, margin, leverage, currency, and other account properties. Use this page to choose the right API for accessing account state.

## 📁 What lives here

* **[AccountSummary](./AccountSummary.md)** — **all account info** at once (login, balance, equity, credit, leverage, currency, margin / free margin / margin level, server time, etc.; no floating profit). Returns `MrpcEnumAccountTradeMode` and `MrpcEnumAccountMarginMode` enums.
* **[AccountInfoDouble](./AccountInfoDouble.md)** — **single double value** from the account (balance, equity, margin, profit, credit, etc.). Uses the `AccountInfoDoublePropertyType` enum as input.
* **[AccountInfoInteger](./AccountInfoInteger.md)** — **single integer value** (login, leverage, limit orders, trade flags, etc.). Uses the `AccountInfoIntegerPropertyType` enum as input.
* **[AccountInfoString](./AccountInfoString.md)** — **single string value** (name, server, currency, company). Uses the `AccountInfoStringPropertyType` enum as input.

---

## 🧭 Plain English

* **AccountSummary** → the **one-stop shop** for a complete account snapshot (balance, equity, leverage, currency, etc.). Returns trade mode as an enum.
* **AccountInfoDouble** → grab **one numeric property** when you need just balance or margin. Requires a property enum as input.
* **AccountInfoInteger** → grab **one integer property** like login number or leverage. Requires a property enum as input.
* **AccountInfoString** → grab **one text property** like account name or currency. Requires a property enum as input.

> Rule of thumb: need the **full snapshot** → `accountSummary()`; need **one specific value** → `accountInfo*` (Double/Integer/String).
>
> **🧱 Enum note:** all 4 methods use enums. `accountInfo*` require a **property enum as input** to select which value to read; `accountSummary()` **returns** the account trade mode (DEMO/CONTEST/REAL) and margin mode as output enums.

---

## Quick choose

| If you need…                                      | Use                   | Returns                   | Enums used                          |
| ------------------------------------------------- | --------------------- | ------------------------- | ----------------------------------- |
| Complete account snapshot (all values)            | `accountSummary`      | `AccountSummaryData`      | **Output:** `MrpcEnumAccountTradeMode`, `MrpcEnumAccountMarginMode` |
| One numeric value (balance, equity, margin, etc.) | `accountInfoDouble`   | single `double`           | **Input:** `AccountInfoDoublePropertyType` |
| One integer value (login, leverage, etc.)         | `accountInfoInteger`  | single `int64_t`          | **Input:** `AccountInfoIntegerPropertyType` |
| One text value (name, currency, server, etc.)     | `accountInfoString`   | single `std::string`      | **Input:** `AccountInfoStringPropertyType` |

---

## ❌ Cross-refs & gotchas

* **Margin** — `accountSummary()` already returns `account_margin()`, `account_margin_free()` and `account_margin_level()` (and `account_margin_mode()`); `accountInfoDouble(ACCOUNT_MARGIN_LEVEL / ACCOUNT_MARGIN_FREE)` is just the single-value alternative.
* **Floating profit** — `accountSummary()` carries balance, equity, leverage **and** the margin fields; the one thing it does *not* carry is **floating profit** — use `accountInfoDouble(ACCOUNT_PROFIT)` for that.
* **accountInfo\*** methods are lighter if you only need one property.
* **Currency** affects how profits are calculated — always check `ACCOUNT_CURRENCY`.
* **Leverage** determines margin requirements — higher leverage = less margin needed.
* **Timeout** — read RPCs use `ClientOptions::unaryTimeoutMs` (default `30000` ms); tune with `account.options()`.

---

## 🟢 Minimal snippets

```cpp
// Complete account snapshot
pb::AccountSummaryData s = account.accountSummary();
std::printf("Balance: %.2f, Equity: %.2f, Leverage: 1:%lld\n",
            s.account_balance(), s.account_equity(), (long long)s.account_leverage());
```

```cpp
// Single property — account balance
pb::AccountInfoDoubleRequest rd;
rd.set_property_id(pb::ACCOUNT_BALANCE);
std::printf("Balance: %.2f\n", account.accountInfoDouble(rd).requested_value());
```

```cpp
// Account leverage
pb::AccountInfoIntegerRequest ri;
ri.set_property_id(pb::ACCOUNT_LEVERAGE);
std::printf("Leverage: 1:%lld\n", (long long)account.accountInfoInteger(ri).requested_value());
```

```cpp
// Account currency
pb::AccountInfoStringRequest rs;
rs.set_property_id(pb::ACCOUNT_CURRENCY);
std::printf("Currency: %s\n", account.accountInfoString(rs).requested_value().c_str());
```

```cpp
// Account health — accountSummary already carries the margin level
pb::AccountSummaryData h = account.accountSummary();
double level = h.account_margin_level();          // also: account_margin(), account_margin_free()
if (level != 0 && level < 100)      std::puts("⚠️ Margin level critical!");
else if (level != 0 && level < 200) std::puts("⚠️ Low margin level");
else                                std::puts("✅ Healthy margin level");
```

---

## 🧱 Enums used in Account Information

### Input enums (property selectors)

| Method               | Input enum                      | Purpose                           | Example values                                           |
| -------------------- | ------------------------------- | --------------------------------- | -------------------------------------------------------- |
| `accountInfoDouble`  | `AccountInfoDoublePropertyType` | Select which double property      | `ACCOUNT_BALANCE`, `ACCOUNT_EQUITY`, `ACCOUNT_MARGIN_FREE`, `ACCOUNT_PROFIT` … (14 total) |
| `accountInfoInteger` | `AccountInfoIntegerPropertyType`| Select which integer property     | `ACCOUNT_LOGIN`, `ACCOUNT_LEVERAGE`, `ACCOUNT_TRADE_MODE`, `ACCOUNT_TRADE_ALLOWED` … (11 total) |
| `accountInfoString`  | `AccountInfoStringPropertyType` | Select which string property      | `ACCOUNT_NAME`, `ACCOUNT_SERVER`, `ACCOUNT_CURRENCY`, `ACCOUNT_COMPANY` (4 total) |

```cpp
pb::AccountInfoDoubleRequest rd; rd.set_property_id(pb::ACCOUNT_BALANCE);
double bal = account.accountInfoDouble(rd).requested_value();

pb::AccountInfoIntegerRequest ri; ri.set_property_id(pb::ACCOUNT_LEVERAGE);
int64_t lev = account.accountInfoInteger(ri).requested_value();

pb::AccountInfoStringRequest rs; rs.set_property_id(pb::ACCOUNT_CURRENCY);
std::string ccy = account.accountInfoString(rs).requested_value();
```

### Output enums (return values)

| Method           | Output enum                 | Accessor               | Values                                   |
| ---------------- | --------------------------- | ---------------------- | ---------------------------------------- |
| `accountSummary` | `MrpcEnumAccountTradeMode`  | `account_trade_mode()` | `MRPC_ACCOUNT_TRADE_MODE_DEMO` (0) / `CONTEST` (1) / `REAL` (2) |
| `accountSummary` | `MrpcEnumAccountMarginMode` | `account_margin_mode()`| `MRPC_ACCOUNT_MARGIN_MODE_RETAIL_NETTING` (0) / `EXCHANGE` (1) / `RETAIL_HEDGING` (2) |

```cpp
pb::AccountSummaryData s = account.accountSummary();
switch (s.account_trade_mode()) {
    case pb::MRPC_ACCOUNT_TRADE_MODE_DEMO: std::puts("Demo account"); break;
    case pb::MRPC_ACCOUNT_TRADE_MODE_REAL: std::puts("Real account"); break;
    default: break;
}
```

### 📝 Important notes about enums

* **Input enums are required** — you must specify which property to read.
* **Namespace-scope values** — in C++ the generated values are *not* `enum class`; write `pb::ACCOUNT_BALANCE`, `pb::MRPC_ACCOUNT_TRADE_MODE_DEMO`.
* **Output enums** — `accountSummary()` returns `account_trade_mode()` / `account_margin_mode()` as typed enum fields.
* **accountInfoInteger special case** — some integer properties *represent* enums (e.g. `ACCOUNT_TRADE_MODE`) but are returned as plain `int64` (0/1/2).

---

## See also

* **Symbol Information:** [SymbolInfoTick](../2.%20Symbol_Information/SymbolInfoTick.md) — get current prices
* **Trading:** [OrderSend](../4.%20Trading_Operations/OrderSend.md) — place orders
* **Positions:** [PositionsTotal](../3.%20Positions_Orders/PositionsTotal.md) — count open positions
