# MT5Sugar — High-Level API (Layer 3 of 3)

`metarpc::MT5Sugar` owns an `MT5Service` and offers the shortest possible one-liners for everyday
tasks: *"what's my balance?"*, *"what's the bid?"*, *"buy 0.1 lots"*, *"close everything"*. No
context, no request objects, no protobuf — prototyping code reads like plain English.

```cpp
#include <metarpc/mt5_sugar.hpp>   // protobuf-free
```

When you outgrow it, drop down to `service()` (friendly facade) or `account()` (raw RPCs).

## Construction & connection

```cpp
using namespace metarpc;
MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "password", "MetaQuotes-Demo");
// or, from MT5_LOGIN / MT5_PASSWORD / MT5_SERVER env vars:
// sugar.quickConnect();

std::cout << "balance " << sugar.getBalance() << ",  EURUSD bid " << sugar.getBid("EURUSD") << "\n";
```

| Method | Purpose |
|--------|---------|
| `connect(login, password, server, name="", timeoutSeconds=120)` | connect |
| `quickConnect()` | connect from `MT5_LOGIN` / `MT5_PASSWORD` / `MT5_SERVER` |
| `isConnected()` / `ping()` / `disconnect()` | status / health / drop channel |
| `service()` / `account()` | reach Layer 2 / Layer 1 |

## Balance & margin

```cpp
double getBalance();  double getEquity();      double getMargin();
double getFreeMargin();  double getMarginLevel();  double getProfit();
```

## Prices & quotes

| Method | Returns |
|--------|---------|
| `getBid(symbol)` / `getAsk(symbol)` / `getSpread(symbol)` | `double` |
| `getPriceInfo(symbol)` | `PriceInfo` (bid/ask/spread/time) |
| `waitForPrice(symbol, targetBid, timeoutMs=10000)` | `PriceInfo` (polls until reached) |

## Trading

Market/pending helpers return the resulting ticket (`0` on a rejected order).

| Method | |
|--------|--|
| `buyMarket(symbol, volume)` / `sellMarket(symbol, volume)` | market orders |
| `buyLimit/sellLimit/buyStop/sellStop(symbol, volume, price)` | pending orders |
| `buyMarketWithSLTP(symbol, volume, sl, tp)` / `sellMarketWithSLTP(...)` | market + absolute SL/TP |
| `buyLimitWithSLTP(symbol, volume, price, sl, tp)` / `sellLimitWithSLTP(...)` | pending + SL/TP |
| `buyMarketWithPips(symbol, volume, slPips, tpPips)` / `sellMarketWithPips(...)` | SL/TP as pip distances |

```cpp
uint64_t ticket = sugar.buyMarket("EURUSD", 0.01);
if (ticket) std::cout << "opened #" << ticket << "\n";
```

## Position management

| Method | |
|--------|--|
| `closePosition(ticket)` / `closePositionPartial(ticket, volume)` | close |
| `closeAllPositions()` / `closeAllBySymbol(symbol)` | returns count closed |
| `modifyPositionSL(ticket, sl)` / `modifyPositionTP(ticket, tp)` / `modifyPositionSLTP(ticket, sl, tp)` | modify |

## Position information

| Method | Returns |
|--------|---------|
| `getOpenPositions()` / `getPositionsBySymbol(symbol)` | `std::vector<Position>` |
| `getPositionByTicket(ticket, bool& found)` | `Position` |
| `hasOpenPosition(symbol)` | `bool` |
| `countOpenPositions()` | `int` |
| `getTotalProfit()` / `getProfitBySymbol(symbol)` | `double` |

## History & statistics

| Method | Returns |
|--------|---------|
| `getDealsToday/Yesterday/ThisWeek/ThisMonth()` | `std::vector<PositionHistory>` |
| `getDealsDateRange(fromUnix, toUnix)` | `std::vector<PositionHistory>` |
| `getProfitToday/ThisWeek/ThisMonth()` | `double` |
| `getDailyStats()` | `DailyStats` (deals, win rate, best/worst) |

## Symbol information

| Method | Returns |
|--------|---------|
| `getSymbolInfo(symbol)` | `SymbolInfo` |
| `getAllSymbols()` | `std::vector<std::string>` |
| `isSymbolAvailable(symbol)` | `bool` |
| `getMinStopLevel(symbol)` | `int64_t` |
| `getSymbolDigits(symbol)` | `int` |

## Risk management

| Method | Returns |
|--------|---------|
| `calculateRequiredMargin(symbol, volume)` | `double` |
| `getMaxLotSize(symbol)` | `double` |
| `calculatePositionSize(symbol, riskPercent, stopLossPips)` | `double` (lots for a % risk) |
| `canOpenPosition(symbol, volume, std::string& reason)` | `bool` |
| `calculateSLTP(symbol, direction, entryPrice, slPips, tpPips, double& sl, double& tp)` | — |

```cpp
double lots = sugar.calculatePositionSize("EURUSD", /*risk%*/ 1.0, /*stopPips*/ 50);
std::string why;
if (sugar.canOpenPosition("EURUSD", lots, why))
    sugar.buyMarketWithPips("EURUSD", lots, 50, 100);
else
    std::cout << "cannot open: " << why << "\n";
```

## Account information

`getAccountInfo()` → `AccountInfo` (full snapshot).

## See also

- [MT5Service](MT5Service.md) — the friendly facade this layer is built on (`sugar.service()`).
- [MT5Account](MT5Account.md) — the raw protobuf layer (`sugar.account()`).
