# ✅ Getting an Account Summary

> **Request:** full account summary (`AccountSummaryData`) from **MT5**. Fetch all core account metrics in a single call.

**API Information:**

* **Low-level API:** `MT5Account::accountSummary(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `AccountSummary` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `AccountSummary(AccountSummaryRequest) → AccountSummaryReply`
* **Low-level client (generated):** `AccountHelper::Stub::AccountSummary(context, request, response)`

---

## 💬 Just the essentials

* **What it is.** A single RPC returning full account state: balance, equity, credit, currency, leverage, trade mode, margin / free margin / margin level / margin mode, and server time.
* **Why you need it.** Fast dashboard/CLI status; double-check login/currency/leverage; heartbeat via `server_time()`.
* **Sanity check.** If you see `account_login()`, `account_currency()`, `account_leverage()`, `account_equity()` → the connection is alive.

---

## 🎯 Purpose

Use it to display real-time account state and sanity-check connectivity:

* Dashboard/CLI status in one call.
* Verify balance & equity before trading.
* Terminal heartbeat via `server_time()` / `server_time_utc()` and `server_utc_offset_seconds()`.

---

## 🧱 Signature

```cpp
namespace metarpc {

class MT5Account {
    // ...
public:
    // Retrieves full account information in a single call.
    // Returns AccountSummaryData with balance, equity, leverage, currency and
    // other account properties. Throws ApiError / TransportError on failure.
    pb::AccountSummaryData accountSummary(const pb::AccountSummaryRequest& req = {});
};

} // namespace metarpc
```

`namespace pb = mt5_term_api;` — the generated protobuf namespace.

**Request message:**

`AccountSummaryRequest {}` *(empty message — just pass the default)*

**Reply message:**

```protobuf
AccountSummaryReply {
  oneof response {
    AccountSummaryData data  = 1;
    Error              error = 2;
  }
}
```

`MT5Account` unwraps the `oneof`: it returns `data` on success and throws `ApiError` (carrying the server-side `Error`) otherwise.

---

## 🔽 Input

| Parameter | Type                         | Description                                              |
| --------- | ---------------------------- | ------------------------------------------------------- |
| `req`     | `const pb::AccountSummaryRequest&` | Empty request message. Optional — defaults to `{}`. |

**There is no `ctx` parameter.** The per-call deadline comes from `ClientOptions`. Tune it on the account:

```cpp
account.options().unaryTimeoutMs = 5000;   // deadline for ordinary read RPCs (default 30000)
account.options().maxRetries     = 5;       // transient-failure / reconnect retry budget
```

Each call runs on a background deadline; on `UNAVAILABLE` / `DEADLINE_EXCEEDED` it is retried with exponential back-off, and a dropped terminal session is re-attached automatically before the retry.

---

## ⬆️ Output — `AccountSummaryData`

| Field (proto)                        | C++ accessor                               | C++ type   | Description                                      |
| ------------------------------------ | ------------------------------------------ | ---------- | ------------------------------------------------ |
| `AccountLogin`                       | `account_login()`                          | `int64_t`  | Trading account login (ID).                      |
| `AccountBalance`                     | `account_balance()`                        | `double`   | Balance excluding floating P/L.                  |
| `AccountEquity`                      | `account_equity()`                         | `double`   | Equity = balance + floating P/L.                 |
| `AccountUserName`                    | `account_user_name()`                      | `const std::string&` | Account holder name.                   |
| `AccountLeverage`                    | `account_leverage()`                       | `int64_t`  | Leverage (e.g., `100` for 1:100).                |
| `AccountTradeMode`                   | `account_trade_mode()`                     | `MrpcEnumAccountTradeMode` | See enum below.                 |
| `AccountCompanyName`                 | `account_company_name()`                   | `const std::string&` | Broker/company display name.           |
| `AccountCurrency`                    | `account_currency()`                       | `const std::string&` | Deposit currency (e.g., `"USD"`).      |
| `ServerTime`                         | `server_time()`                            | `google::protobuf::Timestamp` | Server time at response, in the broker's time zone. |
| `ServerTimeUtc`                      | `server_time_utc()`                        | `google::protobuf::Timestamp` | The same moment as `server_time()`, in UTC. |
| `UtcTimezoneServerTimeShiftMinutes`  | `utc_timezone_server_time_shift_minutes()` | `int64_t`  | Server offset relative to UTC (minutes).         |
| `AccountCredit`                      | `account_credit()`                         | `double`   | Credit amount.                                   |
| `ServerUtcOffsetSeconds`             | `server_utc_offset_seconds()`              | `int32_t`  | Broker server's offset from UTC in seconds (positive = ahead of UTC); `0` while not connected yet. |
| `AccountMargin`                      | `account_margin()`                         | `double`   | Margin currently used by open positions.         |
| `AccountMarginFree`                  | `account_margin_free()`                    | `double`   | Free margin available for new positions.         |
| `AccountMarginLevel`                 | `account_margin_level()`                   | `double`   | Margin level in percent (`equity / margin × 100`); `0` when no margin is used. |
| `AccountMarginMode`                  | `account_margin_mode()`                    | `MrpcEnumAccountMarginMode` | Margin mode (MT5 demo = `RETAIL_HEDGING`). |
| `AccountTradeAllowed`                | `account_trade_allowed()`                  | `bool`     | Trading allowed for this account.                |
| `AccountTradeExpert`                 | `account_trade_expert()`                   | `bool`     | Automated (Expert Advisor) trading allowed.      |
| `TerminalConnected`                  | `terminal_connected()`                     | `bool`     | Terminal is connected to the broker.             |
| `TerminalTradeAllowed`               | `terminal_trade_allowed()`                 | `bool`     | Trading is allowed in the terminal.              |

> C++ protobuf accessors are **snake_case** and return the value by reference for strings/messages, by value for scalars.

---

## 🧱 Related enums (from proto)

> **💡 Enum Usage Note:** in C++ the generated enum **values live at namespace scope** (they are *not* `enum class`), so you write `pb::MRPC_ACCOUNT_TRADE_MODE_DEMO`, not a prefixed form.

### `MrpcEnumAccountTradeMode`

Defined in `mt5-term-api-account-helper.proto`:

```protobuf
enum MrpcEnumAccountTradeMode {
  MRPC_ACCOUNT_TRADE_MODE_DEMO    = 0;  // demo/practice
  MRPC_ACCOUNT_TRADE_MODE_CONTEST = 1;  // contest
  MRPC_ACCOUNT_TRADE_MODE_REAL    = 2;  // real trading
}
```

**Helper to convert the enum to a string:**

```cpp
const char* tradeModeToString(pb::MrpcEnumAccountTradeMode m) {
    switch (m) {
        case pb::MRPC_ACCOUNT_TRADE_MODE_DEMO:    return "DEMO";
        case pb::MRPC_ACCOUNT_TRADE_MODE_CONTEST: return "CONTEST";
        case pb::MRPC_ACCOUNT_TRADE_MODE_REAL:    return "REAL";
        default:                                  return "UNKNOWN";
    }
}
```

### `MrpcEnumAccountMarginMode`

```protobuf
enum MrpcEnumAccountMarginMode {
  MRPC_ACCOUNT_MARGIN_MODE_RETAIL_NETTING = 0;  // netting (one position per symbol)
  MRPC_ACCOUNT_MARGIN_MODE_EXCHANGE       = 1;  // exchange margin
  MRPC_ACCOUNT_MARGIN_MODE_RETAIL_HEDGING = 2;  // hedging (multiple positions per symbol) — typical MT5
}
```

---

## 🧩 Notes & Tips

* **Automatic reconnection:** every `MT5Account` method runs through an internal `call<>` wrapper that retries transient gRPC errors and re-attaches a dropped terminal session (`TERMINAL_INSTANCE_NOT_FOUND`) before retrying.
* **Default timeout:** read RPCs use `ClientOptions::unaryTimeoutMs` (default `30000` ms).
* **Errors are exceptions:** a server-side rejection throws `metarpc::ApiError`; a transport failure past the retry budget throws `metarpc::TransportError`. There are no error return values.
* **Timestamps:** `server_time()` / `server_time_utc()` are `google.protobuf.Timestamp`; read `.seconds()` for epoch seconds, or use the friendly `MT5Service::accountSummary()` which returns UTC fields already converted.

---

## 🔗 Usage Examples

### 1) Basic usage

```cpp
#include <metarpc/mt5_account.hpp>
#include <cstdio>
using namespace metarpc;
namespace pb = mt5_term_api;

int main() {
    MT5Account account("mt5.mrpc.pro", 443);
    account.connect(5057947437, "password", "MetaQuotes-Demo");

    pb::AccountSummaryData s = account.accountSummary();

    std::printf("Account Login: %lld\n", (long long)s.account_login());
    std::printf("Balance: %.2f %s\n", s.account_balance(), s.account_currency().c_str());
    std::printf("Equity: %.2f\n", s.account_equity());
    std::printf("Leverage: 1:%lld\n", (long long)s.account_leverage());
    return 0;
}
```

### 2) Compact status line for a CLI

```cpp
void printAccountStatus(MT5Account& account) {
    try {
        pb::AccountSummaryData s = account.accountSummary();
        std::printf("Acc %lld | %s | Bal %.2f | Eq %.2f | Lev 1:%lld | Mode %s\n",
                    (long long)s.account_login(),
                    s.account_currency().c_str(),
                    s.account_balance(),
                    s.account_equity(),
                    (long long)s.account_leverage(),
                    tradeModeToString(s.account_trade_mode()));
    } catch (const std::exception& e) {
        std::printf("❌ Error: %s\n", e.what());
    }
}
// Output: Acc 5057947437 | USD | Bal 99806.00 | Eq 99850.25 | Lev 1:100 | Mode DEMO
```

### 3) Server time with timezone shift

```cpp
#include <ctime>

void printServerTime(MT5Account& account) {
    pb::AccountSummaryData s = account.accountSummary();

    std::time_t utc    = (std::time_t)s.server_time_utc().seconds();
    std::time_t broker = (std::time_t)s.server_time().seconds();

    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::gmtime(&utc));
    std::printf("Server time (UTC): %s\n", buf);
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::gmtime(&broker));
    std::printf("Server time (broker): %s\n", buf);
    std::printf("Broker offset from UTC: %+d seconds\n", s.server_utc_offset_seconds());
}
```

### 4) Tightening the deadline for one account

```cpp
MT5Account account("mt5.mrpc.pro", 443);
account.options().unaryTimeoutMs = 2000;   // fail fast after 2 s
account.connect(5057947437, "password", "MetaQuotes-Demo");

try {
    pb::AccountSummaryData s = account.accountSummary();
    std::printf("Balance: %.2f\n", s.account_balance());
} catch (const TransportError& e) {
    std::printf("Timed out / transport error: %s\n", e.what());
}
```

### 5) Structured data model

```cpp
// A clean plain struct for UI / business logic
struct AccountSummaryView {
    int64_t      login;
    std::string  currency;
    double       balance;
    double       equity;
    int64_t      leverage;
    std::string  tradeMode;
    std::string  company;
};

AccountSummaryView fromProto(const pb::AccountSummaryData& d) {
    return AccountSummaryView{
        d.account_login(),
        d.account_currency(),
        d.account_balance(),
        d.account_equity(),
        d.account_leverage(),
        tradeModeToString(d.account_trade_mode()),
        d.account_company_name(),
    };
}

// Usage:
AccountSummaryView v = fromProto(account.accountSummary());
std::printf("%lld %s %.2f\n", (long long)v.login, v.currency.c_str(), v.balance);
```

### 6) Error handling (reconnect is automatic)

```cpp
bool tryGetSummary(MT5Account& account, pb::AccountSummaryData& out) {
    try {
        out = account.accountSummary();   // transient errors already retried inside
        return true;
    } catch (const ApiError& e) {
        std::printf("Server rejected the request: %s\n", e.what());
    } catch (const TransportError& e) {
        std::printf("Transport failed after retries: %s\n", e.what());
    }
    return false;
}
```

---

## 🔧 Common Patterns

### Health check

```cpp
bool isAccountHealthy(MT5Account& account) {
    try {
        pb::AccountSummaryData s = account.accountSummary();
        return s.account_login() > 0 &&
               !s.account_currency().empty() &&
               s.account_leverage() > 0;
    } catch (...) {
        return false;
    }
}
```

### Monitor account changes

```cpp
#include <thread>
#include <chrono>

void monitorAccount(MT5Account& account, std::chrono::seconds interval) {
    for (;;) {
        try {
            pb::AccountSummaryData s = account.accountSummary();
            std::printf("Balance: %.2f | Equity: %.2f\n",
                        s.account_balance(), s.account_equity());
        } catch (const std::exception& e) {
            std::printf("❌ Error: %s\n", e.what());
        }
        std::this_thread::sleep_for(interval);
    }
}
// Usage: monitorAccount(account, std::chrono::seconds(5));
```

---

## 📚 See Also

* [AccountInfoDouble](./AccountInfoDouble.md) — get specific double account properties
* [AccountInfoInteger](./AccountInfoInteger.md) — get specific integer account properties
* [AccountInfoString](./AccountInfoString.md) — get specific string account properties
* **Friendly layer:** `MT5Service::accountSummary()` returns a named `AccountInfo` struct with UTC times.
