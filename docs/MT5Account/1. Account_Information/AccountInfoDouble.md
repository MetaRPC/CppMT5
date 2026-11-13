# ✅ Getting a Single Double Account Property

> **Request:** one floating-point account value (`AccountInfoDoubleData`) from **MT5** — balance, equity, margin, profit, etc. — selected by a property enum.

**API Information:**

* **Low-level API:** `MT5Account::accountInfoDouble(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `AccountInfoDouble` (defined in `mt5-term-api-account-information.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `AccountInfoDouble(AccountInfoDoubleRequest) → AccountInfoDoubleReply`

---

## 💬 Just the essentials

* **What it is.** One RPC that returns a **single `double`** chosen by an `AccountInfoDoublePropertyType` enum.
* **Why you need it.** Lighter than `accountSummary()` when you only need one number (e.g. just the free margin before sizing a trade).
* **Sanity check.** `ACCOUNT_BALANCE` should match the balance from `accountSummary()`.

---

## 🎯 Purpose

* Read one monetary metric without pulling the whole summary.
* Poll a single value cheaply (e.g. `ACCOUNT_EQUITY` on a timer).
* Feed risk math: `ACCOUNT_MARGIN_FREE`, `ACCOUNT_MARGIN_LEVEL`, `ACCOUNT_PROFIT`.

---

## 🧱 Signature

```cpp
// Returns the single double value selected by req.property_id().
// Throws ApiError / TransportError on failure.
pb::AccountInfoDoubleData accountInfoDouble(const pb::AccountInfoDoubleRequest& req);
```

**Request message:**

```protobuf
AccountInfoDoubleRequest {
  AccountInfoDoublePropertyType property_id = 1;
}
```

**Reply message:**

```protobuf
AccountInfoDoubleReply {
  oneof response {
    AccountInfoDoubleData data  = 1;   // { double requested_value = 1; }
    Error                 error = 2;
  }
}
```

---

## 🔽 Input

| Field         | C++ setter / type                                  | Description                        |
| ------------- | -------------------------------------------------- | ---------------------------------- |
| `property_id` | `set_property_id(pb::ACCOUNT_BALANCE)` — `AccountInfoDoublePropertyType` | Which double property to read. |

```cpp
pb::AccountInfoDoubleRequest r;
r.set_property_id(pb::ACCOUNT_MARGIN_FREE);
double freeMargin = account.accountInfoDouble(r).requested_value();
```

---

## ⬆️ Output — `AccountInfoDoubleData`

| Field             | C++ accessor        | C++ type | Description                     |
| ----------------- | ------------------- | -------- | ------------------------------- |
| `RequestedValue`  | `requested_value()` | `double` | The value of the chosen property. |

---

## 🧱 Property enum — `AccountInfoDoublePropertyType`

> Values live at namespace scope: write `pb::ACCOUNT_BALANCE`.

| Constant                     | Value | Meaning                                              |
| ---------------------------- | ----- | ---------------------------------------------------- |
| `ACCOUNT_BALANCE`            | 0     | Account balance (excludes floating P/L).             |
| `ACCOUNT_CREDIT`             | 1     | Credit amount.                                       |
| `ACCOUNT_PROFIT`             | 2     | Current floating profit/loss of open positions.      |
| `ACCOUNT_EQUITY`            | 3     | Equity = balance + floating P/L.                     |
| `ACCOUNT_MARGIN`             | 4     | Margin currently used.                               |
| `ACCOUNT_MARGIN_FREE`        | 5     | Free margin available for new positions.             |
| `ACCOUNT_MARGIN_LEVEL`       | 6     | Margin level in percent (`equity / margin × 100`).   |
| `ACCOUNT_MARGIN_SO_CALL`     | 7     | Margin Call level.                                   |
| `ACCOUNT_MARGIN_SO_SO`       | 8     | Stop Out level.                                      |
| `ACCOUNT_MARGIN_INITIAL`     | 9     | Reserved initial margin.                             |
| `ACCOUNT_MARGIN_MAINTENANCE` | 10    | Reserved maintenance margin.                         |
| `ACCOUNT_ASSETS`             | 11    | Current assets.                                      |
| `ACCOUNT_LIABILITIES`        | 12    | Current liabilities.                                 |
| `ACCOUNT_COMMISSION_BLOCKED` | 13    | Blocked commission.                                  |

---

## 🧩 Notes & Tips

* **Automatic reconnection:** transient gRPC errors are retried and a dropped terminal session is re-attached inside `MT5Account`.
* **Default timeout:** `ClientOptions::unaryTimeoutMs` (default `30000` ms).
* **Errors are exceptions:** `ApiError` (server rejected) / `TransportError` (transport failed past retries).
* **One value only.** If you need several numbers at once, `accountSummary()` is cheaper than many `accountInfoDouble()` calls.

---

## 🔗 Usage Examples

### 1) Read the balance

```cpp
pb::AccountInfoDoubleRequest r;
r.set_property_id(pb::ACCOUNT_BALANCE);
double balance = account.accountInfoDouble(r).requested_value();
std::printf("Balance: %.2f\n", balance);
```

### 2) A tiny helper that wraps the enum

```cpp
double accDouble(MT5Account& a, pb::AccountInfoDoublePropertyType id) {
    pb::AccountInfoDoubleRequest r;
    r.set_property_id(id);
    return a.accountInfoDouble(r).requested_value();
}

double equity    = accDouble(account, pb::ACCOUNT_EQUITY);
double freeMargin = accDouble(account, pb::ACCOUNT_MARGIN_FREE);
double profit    = accDouble(account, pb::ACCOUNT_PROFIT);
std::printf("Eq %.2f | Free %.2f | P/L %.2f\n", equity, freeMargin, profit);
```

### 3) Margin-level guard before sizing a trade

```cpp
double accDouble(MT5Account& a, pb::AccountInfoDoublePropertyType id);  // as above

bool safeToTrade(MT5Account& account) {
    double level = accDouble(account, pb::ACCOUNT_MARGIN_LEVEL);
    if (level == 0) return true;                 // no open margin yet
    if (level < 150) { std::puts("⚠️ margin level too low"); return false; }
    return true;
}
```

### 4) Error handling

```cpp
try {
    double p = accDouble(account, pb::ACCOUNT_PROFIT);
    std::printf("Floating P/L: %.2f\n", p);
} catch (const ApiError& e) {
    std::printf("Server error: %s\n", e.what());
} catch (const TransportError& e) {
    std::printf("Transport error: %s\n", e.what());
}
```

---

## 📚 See Also

* [AccountSummary](./AccountSummary.md) — all account values in one call (incl. the margin fields)
* [AccountInfoInteger](./AccountInfoInteger.md) — single integer property (login, leverage, …)
* [AccountInfoString](./AccountInfoString.md) — single string property (name, currency, …)
* **Friendly layer:** `MT5Service::accountInfoDouble(int propertyId)` takes the id as a plain `int`.
