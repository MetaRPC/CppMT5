# MT5Sugar — Complete API Reference

> **High-level convenience methods** for MetaTrader 5 trading automation in C++.

**🧩 API Layer:** **HIGH-LEVEL**. `MT5Sugar` owns an [MT5Service](../MT5Service/MT5Service.Overview.md) (and the underlying [MT5Account](../MT5Account/MT5Account.Master.Overview.md)) and offers the shortest possible one-liners for everyday tasks — *"what's my balance?"*, *"what's the bid?"*, *"buy 0.1 lots"*, *"close everything"*. No context, no request objects, no protobuf.

`MT5Sugar` covers:

- 🔌 Simplified connection management
- 💰 Balance / margin read-outs
- 📈 Prices & quotes
- 💹 Simple trading + trading with SL/TP (prices or pip distances)
- 💼 Position management (close/modify one, by symbol, or all)
- 📋 Position information & live P&L
- 📊 History & daily statistics
- 🧮 Risk-based position sizing and pre-trade checks

> **Rounding note:** the raw order methods (`buyMarket`, `buyLimitWithSLTP`, `modifyPositionSLTP`, …) send volumes/prices **exactly as given**. The helpers round: `calculateSLTP` and `buy/sellMarketWithPips` round SL/TP to the symbol's digits; `calculatePositionSize` / `getMaxLotSize` round volume down to `volumeStep`.

---

## 📦 Construct & connect

```cpp
#include <metarpc/mt5_sugar.hpp>
using namespace metarpc;

MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "password", "MetaQuotes-Demo");   // or sugar.quickConnect() from env

double bal = sugar.getBalance();
uint64_t ticket = sugar.buyMarket("EURUSD", 0.01);
sugar.closePosition((int64_t)ticket);
sugar.disconnect();
```

* **Header is protobuf-free** — compiles fast.
* **Errors are exceptions:** `ApiError` / `TransportError`.
* **Drop down** when you outgrow it: `sugar.service()` (friendly facade) or `sugar.account()` (raw RPCs).
* Order methods return the **resulting ticket** (`uint64_t`), `0` on a rejected order; management methods return `bool` / counts.

---

## 🔌 [01] Connection — [full docs](./1.%20Connection.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `connect(login, password, server, name="", timeoutSeconds=120)` | `void` | Connect by credentials |
| `quickConnect()` | `void` | Connect from env vars `MT5_LOGIN` / `MT5_PASSWORD` / `MT5_SERVER` |
| `isConnected()` | `bool` | True after a successful connect |
| `ping()` | `bool` | Liveness check (fetches account summary) |
| `disconnect()` | `void` | Drop the channel (terminal keeps running) |
| `service()` / `account()` | `MT5Service&` / `MT5Account&` | Reach the lower layers |

---

## 💰 [02] Balance / Margin — [full docs](./2.%20Balance_Margin.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `getBalance()` | `double` | Account balance |
| `getEquity()` | `double` | Equity (balance + floating P/L) |
| `getMargin()` | `double` | Used margin |
| `getFreeMargin()` | `double` | Free margin |
| `getMarginLevel()` | `double` | Margin level, % |
| `getProfit()` | `double` | Current floating profit |

```cpp
std::printf("Bal %.2f | Eq %.2f | Free %.2f | Level %.1f%%\n",
            sugar.getBalance(), sugar.getEquity(), sugar.getFreeMargin(), sugar.getMarginLevel());
```

---

## 📈 [03] Prices / Quotes — [full docs](./3.%20Prices_Quotes.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `getBid(symbol)` | `double` | Current bid |
| `getAsk(symbol)` | `double` | Current ask |
| `getSpread(symbol)` | `double` | `ask - bid` (price units) |
| `getPriceInfo(symbol)` | `PriceInfo` | bid/ask/spread/time in one call |
| `waitForPrice(symbol, targetBid, timeoutMs=10000)` | `PriceInfo` | Poll until bid ≥ target (0 = any tick); empty symbol on timeout |

```cpp
struct PriceInfo { std::string symbol; double bid, ask, spread; int64_t time; };  // time = UTC seconds

PriceInfo p = sugar.getPriceInfo("EURUSD");
std::printf("%.5f / %.5f (spread %.5f)\n", p.bid, p.ask, p.spread);
```

---

## 💹 [04] Simple Trading — [full docs](./4.%20Simple_Trading.md)

Return the resulting ticket (`0` on rejection).

| Method | Returns | Description |
|--------|---------|-------------|
| `buyMarket(symbol, volume)` | `uint64_t` | Market BUY |
| `sellMarket(symbol, volume)` | `uint64_t` | Market SELL |
| `buyLimit(symbol, volume, price)` | `uint64_t` | Pending BUY LIMIT |
| `sellLimit(symbol, volume, price)` | `uint64_t` | Pending SELL LIMIT |
| `buyStop(symbol, volume, price)` | `uint64_t` | Pending BUY STOP |
| `sellStop(symbol, volume, price)` | `uint64_t` | Pending SELL STOP |

```cpp
uint64_t t = sugar.buyMarket("EURUSD", 0.01);
if (t) std::printf("opened #%llu\n", (unsigned long long)t);
```

---

## 🎯 [05] Trading with SL/TP — [full docs](./5.%20Trading_SLTP.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `buyMarketWithSLTP(symbol, volume, sl, tp)` | `uint64_t` | Market BUY with absolute SL/TP |
| `sellMarketWithSLTP(symbol, volume, sl, tp)` | `uint64_t` | Market SELL with absolute SL/TP |
| `buyLimitWithSLTP(symbol, volume, price, sl, tp)` | `uint64_t` | Pending BUY LIMIT with SL/TP |
| `sellLimitWithSLTP(symbol, volume, price, sl, tp)` | `uint64_t` | Pending SELL LIMIT with SL/TP |
| `buyMarketWithPips(symbol, volume, slPips, tpPips)` | `uint64_t` | Market BUY, SL/TP as pip distances |
| `sellMarketWithPips(symbol, volume, slPips, tpPips)` | `uint64_t` | Market SELL, SL/TP as pip distances |

```cpp
// SL 20 pips, TP 40 pips from entry (rounded to digits for you)
uint64_t t = sugar.buyMarketWithPips("EURUSD", 0.01, 20, 40);
```

---

## 💼 [06] Position Management — [full docs](./6.%20Position_Management.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `closePosition(ticket)` | `bool` | Close a whole position |
| `closePositionPartial(ticket, volume)` | `bool` | Partial close |
| `closeAllPositions()` | `int` | Close every position; returns count closed |
| `closeAllBySymbol(symbol)` | `int` | Close all positions on a symbol |
| `modifyPositionSL(ticket, sl)` | `bool` | Set stop loss only |
| `modifyPositionTP(ticket, tp)` | `bool` | Set take profit only |
| `modifyPositionSLTP(ticket, sl, tp)` | `bool` | Set both SL and TP |

```cpp
int closed = sugar.closeAllBySymbol("EURUSD");
std::printf("closed %d EURUSD position(s)\n", closed);
```

---

## 📋 [07] Position Information — [full docs](./7.%20Position_Information.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `getOpenPositions()` | `std::vector<Position>` | All open positions |
| `getPositionsBySymbol(symbol)` | `std::vector<Position>` | Positions on one symbol |
| `getPositionByTicket(ticket, found&)` | `Position` | One position; `found=false` if none |
| `hasOpenPosition(symbol)` | `bool` | Any position on the symbol? |
| `countOpenPositions()` | `int` | Number of open positions |
| `getTotalProfit()` | `double` | Summed floating profit |
| `getProfitBySymbol(symbol)` | `double` | Floating profit on one symbol |

```cpp
for (const Position& p : sugar.getOpenPositions())
    std::printf("#%lld %s %.2f  P/L %.2f\n",
                (long long)p.ticket, p.symbol.c_str(), p.volume, p.profit);
std::printf("Total P/L: %.2f\n", sugar.getTotalProfit());
```

---

## 📊 [08] History / Statistics — [full docs](./8.%20History_Statistics.md)

Day boundaries are UTC.

| Method | Returns | Description |
|--------|---------|-------------|
| `getDealsToday()` | `std::vector<PositionHistory>` | Closed positions today |
| `getDealsYesterday()` | `std::vector<PositionHistory>` | Closed positions yesterday |
| `getDealsThisWeek()` | `std::vector<PositionHistory>` | This week |
| `getDealsThisMonth()` | `std::vector<PositionHistory>` | This month |
| `getDealsDateRange(fromUnix, toUnix)` | `std::vector<PositionHistory>` | Custom window |
| `getProfitToday()` / `getProfitThisWeek()` / `getProfitThisMonth()` | `double` | Net realized profit |
| `getDailyStats()` | `DailyStats` | Today's win/loss summary |

```cpp
struct DailyStats { int totalDeals, winningDeals, losingDeals;
                    double winRate, totalProfit, bestDeal, worstDeal; };

DailyStats d = sugar.getDailyStats();
std::printf("Today: %d deals, win rate %.0f%%, net %.2f\n",
            d.totalDeals, d.winRate, d.totalProfit);
```

---

## 🔎 [09] Symbol Information — [full docs](./9.%20Symbol_Information.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `getSymbolInfo(symbol)` | `SymbolInfo` | Full params for a symbol |
| `getAllSymbols()` | `std::vector<std::string>` | Every symbol name |
| `isSymbolAvailable(symbol)` | `bool` | Exists and is selectable |
| `getMinStopLevel(symbol)` | `int64_t` | Min SL/TP distance (points) |
| `getSymbolDigits(symbol)` | `int` | Price decimal places |

---

## 🧮 [10] Risk Management — [full docs](./10.%20Risk_Management.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `calculatePositionSize(symbol, riskPercent, stopLossPips)` | `double` | Lot size risking `riskPercent`% of balance |
| `getMaxLotSize(symbol)` | `double` | Largest lot the free margin can open |
| `canOpenPosition(symbol, volume, reason&)` | `bool` | Can a `volume`-lot order open now? `reason` explains a false |
| `calculateRequiredMargin(symbol, volume)` | `double` | Margin to open `volume` lots (buy side) |
| `calculateSLTP(symbol, direction, entryPrice, slPips, tpPips, slOut&, tpOut&)` | `void` | Absolute SL/TP prices from pip distances (`direction` = "buy"/"sell") |

```cpp
// Risk 1% of balance on a 20-pip stop, then open that size with SL/TP
double lots = sugar.calculatePositionSize("EURUSD", 1.0, 20);
std::string why;
if (sugar.canOpenPosition("EURUSD", lots, why))
    sugar.buyMarketWithPips("EURUSD", lots, 20, 40);
else
    std::printf("cannot open: %s\n", why.c_str());
```

---

## 👤 [11] Account Information — [full docs](./11.%20Account_Information.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `getAccountInfo()` | `AccountInfo` | Full account snapshot (see [MT5Service DTOs](../MT5Service/MT5Service.Overview.md)) |

---

## 💡 End-to-end example

```cpp
#include <metarpc/mt5_sugar.hpp>
using namespace metarpc;

int main() {
    MT5Sugar sugar("mt5.mrpc.pro", 443);
    sugar.connect(5057947437, "password", "MetaQuotes-Demo");

    if (!sugar.isSymbolAvailable("EURUSD")) { std::puts("symbol n/a"); return 1; }

    // Size by risk, validate, trade
    double lots = sugar.calculatePositionSize("EURUSD", 1.0 /*%*/, 20 /*pips*/);
    std::string reason;
    if (sugar.canOpenPosition("EURUSD", lots, reason)) {
        uint64_t t = sugar.buyMarketWithPips("EURUSD", lots, 20, 40);
        std::printf("opened #%llu (%.2f lots)\n", (unsigned long long)t, lots);
    } else {
        std::printf("skip: %s\n", reason.c_str());
    }

    std::printf("Equity %.2f | Open P/L %.2f\n", sugar.getEquity(), sugar.getTotalProfit());
    sugar.disconnect();
}
```

---

## 🔧 Best practices

1. **Check tickets.** Order methods return `0` on rejection — test before assuming a fill.
2. **Size with risk helpers.** Prefer `calculatePositionSize` + `canOpenPosition` over hard-coded lots.
3. **Clean up.** `disconnect()` when done (also after a failed connect).
4. **Drop down when needed.** `sugar.service()` / `sugar.account()` for anything not wrapped here.

---

## 📚 Related

* **Mid-level:** [MT5Service Overview](../MT5Service/MT5Service.Overview.md)
* **Low-level:** [MT5Account Master Overview](../MT5Account/MT5Account.Master.Overview.md)
* **Guides:** [Your First Project](../All_Guides/Your_First_Project.md) · [MT5 for Beginners](../All_Guides/MT5_For_Beginners.md)
