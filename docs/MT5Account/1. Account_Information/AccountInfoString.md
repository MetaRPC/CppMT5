# ✅ Getting a Single String Account Property

> **Request:** one text account value (`AccountInfoStringData`) from **MT5** — name, server, currency, company — selected by a property enum.

**API Information:**

* **Low-level API:** `MT5Account::accountInfoString(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `AccountInfoString` (defined in `mt5-term-api-account-information.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `AccountInfoString(AccountInfoStringRequest) → AccountInfoStringReply`

---

## 💬 Just the essentials

* **What it is.** One RPC that returns a **single `string`** chosen by an `AccountInfoStringPropertyType` enum.
* **Why you need it.** Read the account currency, broker name, or server string without the full summary.
* **Sanity check.** `ACCOUNT_CURRENCY` should be a 3-letter code like `"USD"`.

---

## 🎯 Purpose

* Display the account's deposit currency (needed to format money and compute P/L).
* Show the broker/company and server in a status line.
* Label logs with the account name.

---

## 🧱 Signature

```cpp
// Returns the single string value selected by req.property_id().
// Throws ApiError / TransportError on failure.
pb::AccountInfoStringData accountInfoString(const pb::AccountInfoStringRequest& req);
```

**Request / Reply:**

```protobuf
AccountInfoStringRequest { AccountInfoStringPropertyType property_id = 1; }

AccountInfoStringReply {
  oneof response {
    AccountInfoStringData data  = 1;   // { string requested_value = 1; }
    Error                 error = 2;
  }
}
```

---

## 🔽 Input

| Field         | C++ setter / type                                  | Description                     |
| ------------- | -------------------------------------------------- | ------------------------------- |
| `property_id` | `set_property_id(pb::ACCOUNT_CURRENCY)` — `AccountInfoStringPropertyType` | Which string property to read. |

---

## ⬆️ Output — `AccountInfoStringData`

| Field            | C++ accessor        | C++ type             | Description                       |
| ---------------- | ------------------- | -------------------- | --------------------------------- |
| `RequestedValue` | `requested_value()` | `const std::string&` | The value of the chosen property. |

---

## 🧱 Property enum — `AccountInfoStringPropertyType`

| Constant           | Value | Meaning                               |
| ------------------ | ----- | ------------------------------------- |
| `ACCOUNT_NAME`     | 0     | Account holder name.                  |
| `ACCOUNT_SERVER`   | 1     | Trade server name (e.g. `MetaQuotes-Demo`). |
| `ACCOUNT_CURRENCY` | 2     | Deposit currency (e.g. `USD`).        |
| `ACCOUNT_COMPANY`  | 3     | Broker/company display name.          |

---

## 🧩 Notes & Tips

* **Automatic reconnection:** handled inside `MT5Account`.
* **Default timeout:** `ClientOptions::unaryTimeoutMs` (default `30000` ms).
* **Errors are exceptions:** `ApiError` / `TransportError`.
* **Currency matters:** always read `ACCOUNT_CURRENCY` before formatting money or computing tick value — profits are expressed in it.

---

## 🔗 Usage Examples

### 1) Read the currency

```cpp
pb::AccountInfoStringRequest r;
r.set_property_id(pb::ACCOUNT_CURRENCY);
std::string ccy = account.accountInfoString(r).requested_value();
std::printf("Currency: %s\n", ccy.c_str());
```

### 2) A helper + a full identity line

```cpp
std::string accStr(MT5Account& a, pb::AccountInfoStringPropertyType id) {
    pb::AccountInfoStringRequest r;
    r.set_property_id(id);
    return a.accountInfoString(r).requested_value();
}

std::printf("%s @ %s (%s), currency %s\n",
            accStr(account, pb::ACCOUNT_NAME).c_str(),
            accStr(account, pb::ACCOUNT_SERVER).c_str(),
            accStr(account, pb::ACCOUNT_COMPANY).c_str(),
            accStr(account, pb::ACCOUNT_CURRENCY).c_str());
```

### 3) Error handling

```cpp
try {
    std::string server = accStr(account, pb::ACCOUNT_SERVER);
    std::printf("Server: %s\n", server.c_str());
} catch (const ApiError& e) {
    std::printf("Server error: %s\n", e.what());
} catch (const TransportError& e) {
    std::printf("Transport error: %s\n", e.what());
}
```

---

## 📚 See Also

* [AccountSummary](./AccountSummary.md) — name, server, currency and company in one call
* [AccountInfoDouble](./AccountInfoDouble.md) — single double property (balance, margin, …)
* [AccountInfoInteger](./AccountInfoInteger.md) — single integer property (login, leverage, …)
* **Friendly layer:** `MT5Service::accountInfoString(int propertyId)` takes the id as a plain `int`.
