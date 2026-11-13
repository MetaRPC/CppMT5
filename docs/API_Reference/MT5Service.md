# MT5Service — Facade API (Layer 2 of 3)

`metarpc::MT5Service` owns an `MT5Account` and gives you a comfortable, **protobuf-free** API:
friendly C++ structs instead of protobuf messages, plain arguments instead of hand-built
`*Request` objects, and automatic **UTC ↔ broker-server time** conversion. This is the layer
most application code should use directly.

```cpp
#include <metarpc/mt5_service.hpp>   // protobuf-free — compiles fast
```

!!! tip "Why it compiles fast"
    This header does not include the generated protobuf headers (it forward-declares
    `MT5Account`). Reach the low-level layer with `account()` when you need a raw RPC — that
    translation unit then pays the protobuf include cost, nothing else does.

## Construction

```cpp
MT5Service(const std::string& host = "mt5.mrpc.pro", int port = 443,
           const std::string& apiKey = "", const std::string& name = "",
           bool utcTimes = true);
```

- `utcTimes` (default `true`): every time the facade returns is UTC and every time you pass in is
  treated as UTC. Set `false` to work in raw broker-server time.
- `account()` → the underlying `MT5Account` for raw RPCs.

```cpp
using namespace metarpc;
MT5Service svc("mt5.mrpc.pro", 443);
svc.connect(5057947437, "password", "MetaQuotes-Demo");

AccountInfo a = svc.accountSummary();
std::cout << a.login << "  " << a.balance << " " << a.currency << "  1:" << a.leverage << "\n";
```

## Friendly types (returned / accepted)

`AccountInfo`, `Tick`, `SymbolInfo`, `Bar`, `OrderRequest`, `OrderResult`, `Position`,
`PendingOrder`, `OpenedOrders`, `Deal`, `HistoryOrder`, `HistoryItem`, `PositionHistory`,
`MarginRate`, `SessionTime`, `OrderCheckResult`, `BookEntry`, `TickValueSize`, `OpenedTickets`,
plus the streaming-event structs (`TradeEvent`, `TransactionEvent`, `PositionProfitEvent`,
`TicketsSnapshot`). Enums: `TradeMode`, `Timeframe`, `TradeAction`.

Errors are the same exceptions as the low level: `ApiError` / `TransportError`.

## Connection & time

| Method | Returns |
|--------|---------|
| `connect(login, password, server, name="", timeoutSeconds=120)` | `void` |
| `reconnect()` / `isConnected()` / `terminalGuid()` / `disconnect(reason="")` / `close()` | — |
| `options()` | `ClientOptions&` |
| `serverOffsetSeconds()` | `int64_t` |
| `toUtc(serverTime)` / `toServerTime(utc)` | `int64_t` |

## Account

| Method | Returns |
|--------|---------|
| `accountSummary()` | `AccountInfo` |
| `accountInfoDouble(propertyId)` | `double` |
| `accountInfoInteger(propertyId)` | `int64_t` |
| `accountInfoString(propertyId)` | `std::string` |

## Symbols & quotes

| Method | Returns |
|--------|---------|
| `symbolSelect(symbol, select=true)` | `bool` |
| `symbolExist(symbol)` / `symbolIsSynchronized(symbol)` | `bool` |
| `symbolList()` | `std::vector<std::string>` |
| `symbolsTotal(selectedOnly=false)` | `int` |
| `symbolName(index, selectedOnly=false)` | `std::string` |
| `symbolInfoTick(symbol)` | `Tick` (UTC times) |
| `symbolParams(symbol)` | `SymbolInfo` (exact-name lookup) |
| `symbolParamsMany(nameFilter="")` | `std::vector<SymbolInfo>` |
| `symbolInfoDouble/Integer/String(symbol, propertyId)` | one property |
| `symbolInfoMarginRate(symbol, orderType)` | `MarginRate` |
| `symbolInfoSessionQuote/SessionTrade(symbol, dayOfWeek, sessionIndex=0)` | `SessionTime` |
| `tickValueWithSize(symbols)` | `std::vector<TickValueSize>` |

## Bars

| Method | Returns |
|--------|---------|
| `priceHistory(symbol, tf, fromUnix, toUnix, maxBars=0)` | `std::vector<Bar>` (UTC) |
| `lastBars(symbol, tf, count)` | newest N bars |

```cpp
for (const Bar& b : svc.lastBars("EURUSD", Timeframe::H1, 5))
    std::cout << b.time << "  O=" << b.open << " C=" << b.close << "\n";
```

## Trading

| Method | Returns |
|--------|---------|
| `orderSend(OrderRequest)` | `OrderResult` (check `.ok()` / `.retcode`) |
| `orderModify(ticket, sl, tp, price=0, stopLimit=0)` | `OrderResult` |
| `orderClose(ticket, volume=0, slippage=10)` | `OrderResult` (volume 0 = whole position) |
| `openedOrders()` | `OpenedOrders` |
| `positions(magic=-1)` | `std::vector<Position>` |
| `positionsTotal()` | `int` |
| `openedOrdersTickets()` | `OpenedTickets` |
| `orderCheck(OrderRequest)` | `OrderCheckResult` (dry run) |
| `orderCalcMargin(symbol, orderType, volume, price)` | `double` |
| `orderCalcProfit(symbol, orderType, volume, openPrice, closePrice)` | `double` |

```cpp
OrderRequest q;
q.symbol = "EURUSD"; q.action = TradeAction::Buy; q.volume = 0.1;
q.stopLoss = 1.0800; q.takeProfit = 1.0900;
OrderResult r = svc.orderSend(q);
if (r.ok()) std::cout << "opened #" << r.order << "\n";
else        std::cout << "rejected: " << r.retcode << " " << r.retcodeDescription << "\n";
```

### Market depth (DOM)

| Method | Returns |
|--------|---------|
| `marketBookAdd(symbol)` | `bool` — subscribe to the order book |
| `marketBookGet(symbol)` | `std::vector<BookEntry>` |
| `marketBookRelease(symbol)` | `bool` |

## History

| Method | Returns |
|--------|---------|
| `orderHistory(fromUnix, toUnix)` | `std::vector<HistoryItem>` |
| `positionsHistory(fromUnix, toUnix)` | `std::vector<PositionHistory>` |

## Streaming (friendly)

Each `subscribe*/stream*` converts the raw payload to friendly structs (times in UTC) and delivers
them to `callback` on a background thread. The returned `ServiceStream` has `stop()/running()/
count()/lastError()`. Keep it alive to keep receiving.

| Method | Callback payload |
|--------|------------------|
| `subscribeTicks(symbols, cb)` | `Tick` |
| `streamTradeUpdates(cb)` | `TradeEvent` |
| `streamTransactions(cb)` | `TransactionEvent` |
| `streamPositionProfits(cb)` | `PositionProfitEvent` |
| `streamOpenedTickets(cb)` | `TicketsSnapshot` |

```cpp
auto stream = svc.subscribeTicks({"EURUSD"}, [](const Tick& t) {
    std::cout << t.symbol << "  bid=" << t.bid << " ask=" << t.ask << "  t=" << t.time << " (UTC)\n";
});
std::this_thread::sleep_for(std::chrono::seconds(5));
stream->stop();
```

## See also

- [MT5Account](MT5Account.md) — the raw protobuf layer underneath (`svc.account()`).
- [MT5Sugar](MT5Sugar.md) — one-line helpers on top of this facade.
