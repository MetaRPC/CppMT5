# MT5Account — Low-Level API (Layer 1 of 3)

`metarpc::MT5Account` is the thin, 1:1-with-the-wire layer. Every method maps to a single
gRPC RPC: you pass a protobuf `*Request` and get back the protobuf `*Data` payload of the
reply. It owns the gRPC channel, the service stubs, the connection/identity state and the
transport machinery — transient-failure retry with exponential back-off and an automatic
`ConnectEx` re-attach on `TERMINAL_INSTANCE_NOT_FOUND`.

```cpp
#include <metarpc/mt5_account.hpp>   // pulls in the generated protobuf headers
```

!!! note "When to use this layer"
    Use `MT5Account` when you need a field the friendly layers do not expose, or full control
    over the exact request. For everyday work prefer [`MT5Service`](MT5Service.md) (friendly
    structs) or [`MT5Sugar`](MT5Sugar.md) (one-liners), which hide protobuf and compile fast.

## How a call works

- **Protobuf in, protobuf out.** `accountSummary()` returns a `pb::AccountSummaryData`; you read
  fields with the generated accessors (`.account_balance()`, `.account_login()`, …).
- **No UTC conversion.** Times are raw broker server time. `MT5Service` is the layer that converts
  to/from UTC.
- **Errors are exceptions** (never error codes):
    - `metarpc::ApiError` — the gateway/terminal answered with an error payload (bad login,
      unknown symbol, trade rejected, …). Carries `code`, `message`, and the MQL5 codes.
    - `metarpc::TransportError` — a gRPC transport failure that outlived the retry budget.
- **Retry & reconnect are automatic** (see `ClientOptions`). You do not retry yourself.

```cpp
using namespace metarpc;
namespace pb = mt5_term_api;

MT5Account acc("mt5.mrpc.pro", 443, /*apiKey*/ "");
acc.connect(5057947437, "password", "MetaQuotes-Demo");

try {
    pb::AccountSummaryData s = acc.accountSummary();
    std::cout << s.account_login() << "  " << s.account_balance() << " " << s.account_currency() << "\n";
} catch (const ApiError& e) {
    std::cerr << e.code << ": " << e.message << "\n";
}
```

## Construction & identity

```cpp
MT5Account(const std::string& host = "mt5.mrpc.pro", int port = 443,
           const std::string& apiKey = "", const std::string& name = "");
```

| Member | Purpose |
|--------|---------|
| `uuid5(ns, name)` *(static)* | RFC 4122 v5 UUID from a namespace + name — the fleet convention for a stable terminal id |
| `computeDeterministicId(login, password)` *(static)* | Legacy SHA-256 id from credentials |
| `setId(id)` / `getId()` | The terminal id (set **before** `connect()`; keep it stable per account) |
| `setApiKey(k)` / `getApiKey()` | Gateway API key (`""` → `MRPC_API_KEY` env → `"TRIAL"`) |
| `setName(n)` / `getName()` | Terminal instance name |
| `options()` | Mutable `ClientOptions` (deadlines, retries, TLS roots, verbosity) |

## Connection

| Method | Returns | Notes |
|--------|---------|-------|
| `connect(login, password, server, name="", timeoutSeconds=120)` | `void` | ConnectEx by broker server name; stores creds for auto-reconnect; blocks until ready |
| `reconnect()` | `void` | Re-issue ConnectEx (also done automatically on a lost terminal) |
| `isConnected()` | `bool` | |
| `terminalGuid()` | `std::string` | The id the gateway returned |
| `disconnect(reason="")` | `void` | **Stops** the terminal on the gateway — avoid on normal shutdown |
| `close()` | `void` | Drops the local channel only; the terminal keeps running |
| `serverOffsetSeconds()` | `int64_t` | Broker clock minus UTC (cached from one `AccountSummary`) |

## Account

| Method | Returns |
|--------|---------|
| `accountSummary(req = {})` | `pb::AccountSummaryData` — full snapshot; also refreshes the cached server-time offset |
| `accountInfoDouble(req)` | `pb::AccountInfoDoubleData` — one double property (`req.property_id`) |
| `accountInfoInteger(req)` | `pb::AccountInfoIntegerData` — one integer property |
| `accountInfoString(req)` | `pb::AccountInfoStringData` — one string property |

## Symbols & quotes

| Method | Returns |
|--------|---------|
| `symbolSelect(req)` | `pb::SymbolSelectData` — add/remove a symbol from Market Watch |
| `symbolExist(req)` | `pb::SymbolExistData` |
| `symbolIsSynchronized(req)` | `pb::SymbolIsSynchronizedData` |
| `symbolList(req = {})` | `pb::SymbolListData` — every symbol name on the terminal |
| `symbolsTotal(req)` | `pb::SymbolsTotalData` — count (all or Market Watch) |
| `symbolName(req)` | `pb::SymbolNameData` — name by index |
| `symbolInfoTick(req)` | `pb::MrpcMqlTick` — latest tick (note: returns the tick directly) |
| `symbolInfoDouble/Integer/String(req)` | one property by `req.type` (the PROTO `SymbolInfo*Property` enum) |
| `symbolInfoMarginRate(req)` | `pb::SymbolInfoMarginRateData` |
| `symbolInfoSessionQuote/SessionTrade(req)` | session windows (raw server time) |
| `symbolParamsMany(req)` | `pb::SymbolParamsManyData` — full params for one/many symbols |
| `tickValueWithSize(req)` | `pb::TickValueWithSizeData` — tick value + sizing for several symbols |

## Bars

| Method | Returns |
|--------|---------|
| `priceHistory(req)` | `pb::PriceHistoryData` — OHLC bars (window & times are raw server time) |

## Trading

| Method | Returns |
|--------|---------|
| `orderSend(req)` | `pb::OrderSendData` — place a market/pending order (inspect `returned_code`, 10009 = done) |
| `orderModify(req)` | `pb::OrderModifyData` — change SL/TP/price |
| `orderClose(req)` | `pb::OrderCloseData` — close/partial-close or delete a pending |
| `openedOrders(req)` | `pb::OpenedOrdersData` — positions + pending orders snapshot |
| `openedOrdersTickets(req)` | `pb::OpenedOrdersTicketsData` — just the ticket lists |
| `positionsTotal()` | `pb::PositionsTotalData` — open position count (no request) |
| `orderCheck(req)` | `pb::OrderCheckData` — dry-run validation |
| `orderCalcMargin(req)` | `pb::OrderCalcMarginData` |
| `orderCalcProfit(req)` | `pb::OrderCalcProfitData` |

## History

| Method | Returns |
|--------|---------|
| `orderHistory(req)` | `pb::OrdersHistoryData` — closed orders & deals in a window |
| `positionsHistory(req)` | `pb::PositionsHistoryData` — closed positions in a window |

## Streaming

Every `onX()` opens a server stream and delivers raw protobuf payloads to your callback on a
background thread. The returned `AccountStream` owns the subscription — keep it alive to keep
receiving, destroy it (or call `stop()`) to cancel.

| Method | Callback payload |
|--------|------------------|
| `onSymbolTick(req, cb)` | `pb::OnSymbolTickData` |
| `onTrade(req, cb)` | `pb::OnTradeData` |
| `onTradeTransaction(req, cb)` | `pb::OnTradeTransactionData` |
| `onPositionProfit(req, cb)` | `pb::OnPositionProfitData` |
| `onPositionsAndPendingOrdersTickets(req, cb)` | `pb::OnPositionsAndPendingOrdersTicketsData` |

`AccountStream` exposes `stop()`, `running()`, `count()`, `lastError()`.

```cpp
pb::OnSymbolTickRequest req;
req.add_symbol_names("EURUSD");
auto stream = acc.onSymbolTick(req, [](const pb::OnSymbolTickData& d) {
    if (d.has_symbol_tick())
        std::cout << d.symbol_tick().symbol() << " " << d.symbol_tick().bid() << "\n";
});
std::this_thread::sleep_for(std::chrono::seconds(5));
stream->stop();   // (also happens automatically when `stream` is destroyed)
```

## Example — a single symbol property, the raw way

```cpp
using namespace metarpc;
namespace pb = mt5_term_api;

// SymbolInfoIntegerProperty: 17 = SYMBOL_DIGITS
pb::SymbolInfoIntegerRequest req;
req.set_symbol("EURUSD");
req.set_type((pb::SymbolInfoIntegerProperty)17);
int64_t digits = acc.symbolInfoInteger(req).value();
```

## See also

- [MT5Service](MT5Service.md) — the same data as friendly C++ structs, with UTC conversion.
- [MT5Sugar](MT5Sugar.md) — one-line helpers on top of `MT5Service`.
