# ✅ Getting a Single Integer Account Property

> **Request:** one integer account value (`AccountInfoIntegerData`) from **MT5** — login, leverage, limit orders, trade flags — selected by a property enum.

**API Information:**

* **Low-level API:** `MT5Account::accountInfoInteger(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `AccountInfoInteger` (defined in `mt5-term-api-account-information.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `AccountInfoInteger(AccountInfoIntegerRequest) → AccountInfoIntegerReply`

---

## 💬 Just the essentials

* **What it is.** One RPC that returns a **single `int64`** chosen by an `AccountInfoIntegerPropertyType` enum.
* **Why you need it.** Read the login number, leverage, or a boolean-style flag (`ACCOUNT_TRADE_ALLOWED`) without the full summary.
* **Sanity check.** `ACCOUNT_LOGIN` should equal the login you connected with.

---

## 🎯 Purpose

* Read leverage before computing required margin.
* Check whether trading / Expert Advisors are allowed.
* Detect the account trade mode (`ACCOUNT_TRADE_MODE`: 0 = DEMO, 1 = CONTEST, 2 = REAL).

---

## 🧱 Signature

```cpp
// Returns the single int64 value selected by req.property_id().
// Throws ApiError / TransportError on failure.
pb::AccountInfoIntegerData accountInfoInteger(const pb::AccountInfoIntegerRequest& req);
```

**Request / Reply:**

```protobuf
AccountInfoIntegerRequest { AccountInfoIntegerPropertyType property_id = 1; }

AccountInfoIntegerReply {
  oneof response {
    AccountInfoIntegerData data  = 1;   // { int64 requested_value = 1; }
    Error                  error = 2;
  }
}
```

---

## 🔽 Input

| Field         | C++ setter / type                                   | Description                      |
| ------------- | --------------------------------------------------- | -------------------------------- |
| `property_id` | `set_property_id(pb::ACCOUNT_LEVERAGE)` — `AccountInfoIntegerPropertyType` | Which integer property to read. |

---

## ⬆️ Output — `AccountInfoIntegerData`

| Field            | C++ accessor        | C++ type  | Description                       |
| ---------------- | ------------------- | --------- | --------------------------------- |
| `RequestedValue` | `requested_value()` | `int64_t` | The value of the chosen property. |

> Flag-style properties (`ACCOUNT_TRADE_ALLOWED`, `ACCOUNT_TRADE_EXPERT`, `ACCOUNT_FIFO_CLOSE`, `ACCOUNT_HEDGE_ALLOWED`) return `0` / `1`. Enum-style properties (`ACCOUNT_TRADE_MODE`, `ACCOUNT_MARGIN_MODE`) return the enum's integer value.

---

## 🧱 Property enum — `AccountInfoIntegerPropertyType`

| Constant                 | Value | Meaning                                                        |
| ------------------------ | ----- | -------------------------------------------------------------- |
| `ACCOUNT_LOGIN`          | 0     | Account login (ID).                                            |
| `ACCOUNT_TRADE_MODE`     | 1     | Account type: 0 = DEMO, 1 = CONTEST, 2 = REAL.                 |
| `ACCOUNT_LEVERAGE`       | 2     | Leverage (e.g. `100` = 1:100).                                 |
| `ACCOUNT_LIMIT_ORDERS`   | 3     | Max number of pending orders allowed.                         |
| `ACCOUNT_MARGIN_SO_MODE` | 4     | Mode for Stop Out level (percent vs money).                    |
| `ACCOUNT_TRADE_ALLOWED`  | 5     | Trading allowed for this account (0/1).                        |
| `ACCOUNT_TRADE_EXPERT`   | 6     | Automated (EA) trading allowed (0/1).                          |
| `ACCOUNT_MARGIN_MODE`    | 7     | Margin mode: 0 = NETTING, 1 = EXCHANGE, 2 = RETAIL_HEDGING.    |
| `ACCOUNT_CURRENCY_DIGITS`| 8     | Number of decimal digits for the account currency.            |
| `ACCOUNT_FIFO_CLOSE`     | 9     | Positions must be closed FIFO (0/1).                           |
| `ACCOUNT_HEDGE_ALLOWED`  | 10    | Hedging (opposite positions on one symbol) allowed (0/1).      |

---

## 🧩 Notes & Tips

* **Automatic reconnection:** handled inside `MT5Account`.
* **Default timeout:** `ClientOptions::unaryTimeoutMs` (default `30000` ms).
* **Errors are exceptions:** `ApiError` / `TransportError`.
* **Enums as int:** when you query `ACCOUNT_TRADE_MODE` the value is returned as `int64` (`0`/`1`/`2`), not a typed enum — compare against the numeric values.

---

## 🔗 Usage Examples

### 1) Read leverage

```cpp
pb::AccountInfoIntegerRequest r;
r.set_property_id(pb::ACCOUNT_LEVERAGE);
int64_t leverage = account.accountInfoInteger(r).requested_value();
std::printf("Leverage: 1:%lld\n", (long long)leverage);
```

### 2) A helper + the trade-mode check

```cpp
int64_t accInt(MT5Account& a, pb::AccountInfoIntegerPropertyType id) {
    pb::AccountInfoIntegerRequest r;
    r.set_property_id(id);
    return a.accountInfoInteger(r).requested_value();
}

int64_t mode = accInt(account, pb::ACCOUNT_TRADE_MODE);
std::printf("Account is %s\n", mode == 0 ? "DEMO" : mode == 2 ? "REAL" : "CONTEST");
```

### 3) Guard: refuse to trade on a real account

```cpp
int64_t accInt(MT5Account& a, pb::AccountInfoIntegerPropertyType id);  // as above

if (accInt(account, pb::ACCOUNT_TRADE_MODE) == 2 /* REAL */) {
    throw std::runtime_error("Refusing to run a test strategy on a REAL account");
}
if (!accInt(account, pb::ACCOUNT_TRADE_ALLOWED)) {
    std::puts("Trading is disabled for this account");
}
```

---

## 📚 See Also

* [AccountSummary](./AccountSummary.md) — login, leverage, trade mode and more in one call
* [AccountInfoDouble](./AccountInfoDouble.md) — single double property (balance, margin, …)
* [AccountInfoString](./AccountInfoString.md) — single string property (name, currency, …)
* **Friendly layer:** `MT5Service::accountInfoInteger(int propertyId)` takes the id as a plain `int`.
