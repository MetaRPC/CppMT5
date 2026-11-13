# Return Codes & Error Handling

CppMT5 reports failures two ways: **exceptions** for failed RPCs, and a numeric **trade retcode**
inside a successful trade reply. Understand both.

## Exceptions (RPC-level)

Every method throws on failure — there are no error-code return values.

```cpp
try {
    sugar.buyMarket("EURUSD", 0.1);
} catch (const metarpc::ApiError& e) {
    // the gateway/terminal said no
    std::cerr << e.code << ": " << e.message
              << "  (mql " << e.mqlErrorCode << ", trade " << e.mqlTradeCode << ")\n";
    if (e.isInvalidLogin()) { /* bad credentials — do not retry */ }
} catch (const metarpc::TransportError& e) {
    // transport failed past the retry budget
    std::cerr << "transport " << e.grpcCode << ": " << e.what() << "\n";
}
```

| Exception | When | Key fields |
|-----------|------|-----------|
| `ApiError` | the server understood the request and returned an error payload (bad login, unknown symbol, trade rejected, terminal not found) | `code`, `message`, `mqlErrorCode`, `mqlTradeCode`, `where`; helpers `isInvalidLogin()`, `isTerminalNotFound()` |
| `TransportError` | a gRPC transport failure survived all retries | `grpcCode` (14 = UNAVAILABLE, 4 = DEADLINE_EXCEEDED) |

What is **not** an exception, because the SDK handles it for you:

- Transient transport failures → retried with exponential back-off (`ClientOptions::maxRetries`).
- `TERMINAL_INSTANCE_NOT_FOUND` → automatic `ConnectEx` re-attach, then retry.

## Trade retcodes (order-level)

A trade RPC can *succeed as an RPC* and still *reject the trade*. Always inspect the retcode:

```cpp
OrderResult r = sugar.service().orderSend(req);
if (r.ok()) std::cout << "done, deal #" << r.deal << "\n";       // ok() = 10009 / 10008 / 10010
else        std::cout << "rejected " << r.retcode << ": " << r.retcodeDescription << "\n";
```

`OrderResult::ok()` is true for the accepted codes (done / placed / done-partial). The ones you
will meet most:

| Code | Name | Meaning |
|------|------|---------|
| **10009** | `TRADE_RETCODE_DONE` | **Request completed — success** |
| 10008 | `TRADE_RETCODE_PLACED` | Pending order placed |
| 10010 | `TRADE_RETCODE_DONE_PARTIAL` | Only part of the volume filled |
| 10004 | `TRADE_RETCODE_REQUOTE` | Price changed — retry with a fresh price |
| 10006 | `TRADE_RETCODE_REJECT` | Request rejected |
| 10013 | `TRADE_RETCODE_INVALID` | Invalid request |
| 10014 | `TRADE_RETCODE_INVALID_VOLUME` | Volume below min / above max / off step |
| 10015 | `TRADE_RETCODE_INVALID_PRICE` | Invalid price |
| 10016 | `TRADE_RETCODE_INVALID_STOPS` | SL/TP too close (below the symbol's stops level) |
| 10018 | `TRADE_RETCODE_MARKET_CLOSED` | Market is closed |
| 10019 | `TRADE_RETCODE_NO_MONEY` | Not enough free margin |
| 10027 | `TRADE_RETCODE_CLIENT_DISABLES_AT` | Autotrading disabled on the client |

`orderCheck()` returns the same kind of retcode without sending anything — use it to pre-flight a
trade (`0` means it would pass).

## A robust trade pattern

```cpp
try {
    OrderRequest q; q.symbol = "EURUSD"; q.action = TradeAction::Buy; q.volume = 0.1;

    auto check = sugar.service().orderCheck(q);
    if (!check.ok()) { std::cerr << "pre-check failed: " << check.comment << "\n"; return; }

    auto r = sugar.service().orderSend(q);
    if (!r.ok()) std::cerr << "send rejected: " << r.retcode << " " << r.retcodeDescription << "\n";
}
catch (const metarpc::ApiError& e)       { std::cerr << "API: " << e.what() << "\n"; }
catch (const metarpc::TransportError& e) { std::cerr << "transport: " << e.what() << "\n"; }
```

## Where the two meet

Some trade failures surface as an **`ApiError`** rather than a retcode on a returned `OrderResult`
(for example when the terminal rejects the request outright). In that case the MQL trade code is
on the exception as **`mqlTradeCode`**, and the symbolic code is in `code`:

```cpp
try {
    sugar.service().orderSend(q);
} catch (const metarpc::ApiError& e) {
    if (e.mqlTradeCode == 10019) std::cerr << "not enough money\n";   // same 10019 as the table
    else std::cerr << e.code << ": " << e.message << "\n";
}
```

So: inspect `OrderResult::retcode` when a call **returns**, and `ApiError::mqlTradeCode` when it
**throws** — they speak the same numbering.

## 📚 See also

* [OrderSend](../MT5Account/4.%20Trading_Operations/OrderSend.md) · [OrderCheck](../MT5Account/4.%20Trading_Operations/OrderCheck.md) — where retcodes come from
* [Trading Operations overview](../MT5Account/4.%20Trading_Operations/Trading_Operations.Overview.md) · [MT5Service Trading Methods](../MT5Service/4.%20Trading_Methods.md)
* [Enums Reference](ENUMS_USAGE_REFERENCE.md) · [Glossary](GLOSSARY.md)
