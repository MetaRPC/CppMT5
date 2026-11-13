# ✅ Streaming the Trade-Transaction Log

> **Subscribe:** receive a callback for every trade transaction (`OnTradeTransactionData`) — the complete, low-level audit trail of request → transaction → result.

**API Information:**

* **Low-level API:** `MT5Account::onTradeTransaction(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.SubscriptionService`
* **Proto definition:** `OnTradeTransaction` (defined in `mt5-term-api-subscriptions.proto`)

### RPC

* **Service:** `mt5_term_api.SubscriptionService`
* **Method:** `OnTradeTransaction(OnTradeTransactionRequest) → stream OnTradeTransactionData`

---

## 💬 Just the essentials

* **What it is.** The most detailed trade stream: each message carries the originating **request**, the **transaction**, and the **result**.
* **Why you need it.** Precise audit/reconciliation — matching exactly what the server did to each request.
* **Lifecycle.** Returns a `std::unique_ptr<AccountStream>` — keep it alive; `stop()` to end.

---

## 🧱 Signature

```cpp
// Streams the detailed transaction log to `callback` on a background thread.
// Throws ApiError / TransportError while opening.
std::unique_ptr<AccountStream> onTradeTransaction(
    const pb::OnTradeTransactionRequest& req,
    std::function<void(const pb::OnTradeTransactionData&)> callback);
```

**Stream payload:**

```protobuf
OnTradeTransactionData {
  MT5_SUB_ENUM_EVENT_GROUP_TYPE type                      = 1;
  MqlTradeTransaction           trade_transaction         = 2;
  MqlTradeRequest               trade_request             = 3;
  MqlTradeResult                trade_result              = 4;
  OnEventAccountInfo            account_info              = 5;
  string                        terminal_instance_guid_id = 6;
}
```

---

## ⬆️ Payload — `OnTradeTransactionData`

| Accessor               | Type                            | Description                               |
| ---------------------- | ------------------------------- | ----------------------------------------- |
| `type()`               | `MT5_SUB_ENUM_EVENT_GROUP_TYPE` | Event group/type.                         |
| `trade_transaction()`  | `MqlTradeTransaction`           | The transaction (deal/order/position changes). |
| `trade_request()`      | `MqlTradeRequest`               | The originating request (if any).         |
| `trade_result()`       | `MqlTradeResult`                | The server's result (retcode, deal, order). |
| `account_info()`       | `OnEventAccountInfo`            | Account snapshot at the event.            |
| `terminal_instance_guid_id()` | `string`                 | Terminal session id.                      |

---

## 🧩 Notes & Tips

* **Finer-grained than [OnTrade](./OnTrade.md).** Use this when you need the request/result pairing; use `OnTrade` for simpler "what's open changed" logic.
* **Callback on the reader thread** — don't block; guard shared state.
* **Keep the handle alive**; `stop()` to end. Handle API: `running()`, `count()`, `lastError()`.
* See [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md).

---

## 🔗 Usage Example

```cpp
#include <thread>
#include <chrono>

pb::OnTradeTransactionRequest req;
auto stream = account.onTradeTransaction(req, [](const pb::OnTradeTransactionData& d) {
    std::printf("txn type=%d, result retcode=%u\n",
                (int)d.type(), d.trade_result().trade_return_code());
});

std::this_thread::sleep_for(std::chrono::minutes(1));
stream->stop();
```

---

## 🧱 Enums used (in the returned data)

| Field | Enum | Values |
|-------|------|--------|
| `type()` | `MT5_SUB_ENUM_EVENT_GROUP_TYPE` | `OrderProfit`(0), `OrderUpdate`(1) |

`trade_result().trade_return_code()` is a trade retcode (`10009` = DONE), not an enum. The embedded `MqlTradeRequest` / `MqlTradeTransaction` carry order-type / filling / time enums — full catalog: [Enums Reference](../../All_Guides/ENUMS_USAGE_REFERENCE.md) · [Return Codes](../../All_Guides/RETURN_CODES_REFERENCE.md).

## 📚 See Also

* [OnTrade](./OnTrade.md) — higher-level position/order change events
* [OrderSend](../4.%20Trading_Operations/OrderSend.md) — the requests that produce these transactions
* [gRPC Streaming](../../All_Guides/GRPC_STREAM_MANAGEMENT.md)
* **Friendly layer:** `MT5Service::streamTransactions(cb)` delivers named `TransactionEvent` structs.
