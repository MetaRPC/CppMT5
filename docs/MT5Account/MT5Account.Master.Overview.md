# MT5Account — Master Overview

> One page to **orient fast**: what lives where, how to choose the right API, and jump links to every **overview** and **method spec** in this docs set.

---

## 🚦 Start here — Section Overviews

* **[Account Information — Overview](./1.%20Account_Information/Account_Information.Overview.md)**
  Account balance/equity/margin/leverage — complete snapshot or single properties.

* **[Symbol Information — Overview](./2.%20Symbol_Information/Symbol_Information.Overview.md)**
  Quotes, symbol properties, trading rules, Market Watch management.

* **[Positions & Orders — Overview](./3.%20Positions_Orders/Positions_Orders.Overview.md)**
  Open positions, pending orders, historical deals, order history.

* **[Trading Operations — Overview](./4.%20Trading_Operations/Trading_Operations.Overview.md)**
  Order execution, position management, margin calculations, trade validation.

* **[Market Depth (DOM) — Overview](./5.%20Market_Depth(DOM)/Market_Depth.Overview.md)**
  Level II quotes, order book data, market depth subscription.

* **[Additional Methods — Overview](./6.%20Additional_Methods/Additional_Methods.Overview.md)**
  Advanced symbol info, trading sessions, margin rates, bar history.

* **[Streaming Methods — Overview](./7.%20Streaming_Methods/Streaming_Methods.Overview.md)**
  Real-time streams: ticks, trades, profit updates, transaction log.

---

## 🧭 How to pick an API

| If you need…                    | Go to…                    | Typical calls                                                    |
| ------------------------------- | ------------------------- | --------------------------------------------------------------- |
| Account snapshot                | Account Information       | `accountSummary`, `accountInfoDouble`, `accountInfoInteger`     |
| Quotes & symbol properties      | Symbol Information        | `symbolInfoTick`, `symbolInfoDouble`, `symbolsTotal`            |
| Current positions & orders      | Positions & Orders        | `positionsTotal`, `openedOrders`, `openedOrdersTickets`         |
| Historical trades               | Positions & Orders        | `orderHistory`, `positionsHistory`                              |
| Level II / Order book           | Market Depth (DOM)        | `marketBookAdd`, `marketBookGet`, `marketBookRelease`           |
| Trading operations              | Trading Operations        | `orderSend`, `orderModify`, `orderClose`                        |
| Pre-trade calculations          | Trading Operations        | `orderCalcMargin`, `orderCheck`                                 |
| Advanced symbol data / bars     | Additional Methods        | `symbolParamsMany`, `symbolInfoSessionTrade`, `priceHistory`    |
| Real-time updates               | Streaming Methods         | `onSymbolTick`, `onTrade`, `onPositionProfit`                   |

---

## 🔌 Usage pattern (raw gRPC)

Every method is called on an `MT5Account` (`#include <metarpc/mt5_account.hpp>`, namespace `metarpc`). It owns the gRPC channel, sends the terminal id and API key as metadata, retries transient failures, re-attaches a dropped terminal session, and unwraps the reply — returning the protobuf `*Data` payload or throwing.

```cpp
#include <metarpc/mt5_account.hpp>
using namespace metarpc;
namespace pb = mt5_term_api;              // the generated protobuf namespace

// Create the account (prepares the gRPC channel)
MT5Account account("mt5.mrpc.pro", 443);

// Connect the terminal (login, password, broker server)
account.connect(5057947437, "password", "MetaQuotes-Demo");
// The gateway returns a terminal session GUID; later calls reuse it automatically.

// Call a method: returns the Data payload, or throws ApiError / TransportError
pb::AccountSummaryData s = account.accountSummary();
std::printf("Balance: $%.2f\n", s.account_balance());
```

---

Every method follows the same shape:

* **Proto Service/Method:** `Service.Method(Request) → Reply`
* **C++ call:** `account.method(const pb::Request&)` returns `pb::MethodData` (by value)
* **Reply structure:** the gRPC reply carries either a `Data` payload or an `Error`; `MT5Account` returns the `Data` and turns `Error` into a thrown **`ApiError`** (server-side error) — transport failures past the retry budget throw **`TransportError`**
* **Return codes:** trading operations return status codes (`10009` = success / DONE)

**Timestamps:** proto uses `google.protobuf.Timestamp`. The raw layer gives you broker/server time; the friendly `MT5Service` layer converts to UTC for you.

**Streaming methods:** return a `std::unique_ptr<AccountStream>`. The SDK reads the stream on a **background thread** and invokes your callback for each message; keep the handle alive and call `stop()` to end it.

---

# 📚 Full Index · All Method Specs

---

## 📄 Account Information

* **Overview:** [Account_Information.Overview.md](./1.%20Account_Information/Account_Information.Overview.md)

### Complete Snapshot

* [AccountSummary.md](./1.%20Account_Information/AccountSummary.md) — all account info at once (balance, equity, margin, etc.)

### Individual Properties

* [AccountInfoDouble.md](./1.%20Account_Information/AccountInfoDouble.md) — single double value (balance, equity, margin, profit, etc.)
* [AccountInfoInteger.md](./1.%20Account_Information/AccountInfoInteger.md) — single integer value (login, leverage, limit orders, etc.)
* [AccountInfoString.md](./1.%20Account_Information/AccountInfoString.md) — single string value (name, server, currency, company)

---

## 📊 Symbol Information

* **Overview:** [Symbol_Information.Overview.md](./2.%20Symbol_Information/Symbol_Information.Overview.md)

### Current Quotes

* [SymbolInfoTick.md](./2.%20Symbol_Information/SymbolInfoTick.md) — current quote for symbol (bid, ask, last, volume, time)

### Symbol Inventory & Management

* [SymbolsTotal.md](./2.%20Symbol_Information/SymbolsTotal.md) — count of available symbols
* [SymbolName.md](./2.%20Symbol_Information/SymbolName.md) — get symbol name by index
* [SymbolList.md](./2.%20Symbol_Information/SymbolList.md) — list symbol names in one call
* [SymbolSelect.md](./2.%20Symbol_Information/SymbolSelect.md) — enable/disable symbol in Market Watch
* [SymbolExist.md](./2.%20Symbol_Information/SymbolExist.md) — check if symbol exists
* [SymbolIsSynchronized.md](./2.%20Symbol_Information/SymbolIsSynchronized.md) — check symbol data sync status

### Symbol Properties

* [SymbolInfoDouble.md](./2.%20Symbol_Information/SymbolInfoDouble.md) — single double property (bid, ask, point, volume min/max, etc.)
* [SymbolInfoInteger.md](./2.%20Symbol_Information/SymbolInfoInteger.md) — single integer property (digits, spread, stops level, etc.)
* [SymbolInfoString.md](./2.%20Symbol_Information/SymbolInfoString.md) — single string property (description, currency, path)

---

## 📦 Positions & Orders

* **Overview:** [Positions_Orders.Overview.md](./3.%20Positions_Orders/Positions_Orders.Overview.md)

### Current State

* [PositionsTotal.md](./3.%20Positions_Orders/PositionsTotal.md) — count of open positions
* [OpenedOrders.md](./3.%20Positions_Orders/OpenedOrders.md) — full details of all open positions and pending orders
* [OpenedOrdersTickets.md](./3.%20Positions_Orders/OpenedOrdersTickets.md) — ticket numbers only (lightweight)

### Historical Data

* [OrderHistory.md](./3.%20Positions_Orders/OrderHistory.md) — historical orders within a time range (with pagination)
* [PositionsHistory.md](./3.%20Positions_Orders/PositionsHistory.md) — closed positions within a time range (with pagination)

---

## 🛠 Trading Operations

* **Overview:** [Trading_Operations.Overview.md](./4.%20Trading_Operations/Trading_Operations.Overview.md)

### Order Execution & Management

* [OrderSend.md](./4.%20Trading_Operations/OrderSend.md) — place market or pending orders
* [OrderModify.md](./4.%20Trading_Operations/OrderModify.md) — modify SL/TP or order parameters
* [OrderClose.md](./4.%20Trading_Operations/OrderClose.md) — close positions (full or partial)

### Pre-Trade Calculations

* [OrderCalcMargin.md](./4.%20Trading_Operations/OrderCalcMargin.md) — calculate margin required for a trade
* [OrderCalcProfit.md](./4.%20Trading_Operations/OrderCalcProfit.md) — calculate potential profit
* [OrderCheck.md](./4.%20Trading_Operations/OrderCheck.md) — validate a trade request before execution

---

## 📈 Market Depth (DOM)

* **Overview:** [Market_Depth.Overview.md](./5.%20Market_Depth(DOM)/Market_Depth.Overview.md)

### Level II Quotes

* [MarketBookAdd.md](./5.%20Market_Depth(DOM)/MarketBookAdd.md) — subscribe to Market Depth for a symbol
* [MarketBookGet.md](./5.%20Market_Depth(DOM)/MarketBookGet.md) — get current order book data
* [MarketBookRelease.md](./5.%20Market_Depth(DOM)/MarketBookRelease.md) — unsubscribe from Market Depth

---

## 🔧 Additional Methods

* **Overview:** [Additional_Methods.Overview.md](./6.%20Additional_Methods/Additional_Methods.Overview.md)

### Advanced Symbol Information

* [SymbolInfoMarginRate.md](./6.%20Additional_Methods/SymbolInfoMarginRate.md) — margin rates for a symbol and order type
* [SymbolInfoSessionQuote.md](./6.%20Additional_Methods/SymbolInfoSessionQuote.md) — quote session times
* [SymbolInfoSessionTrade.md](./6.%20Additional_Methods/SymbolInfoSessionTrade.md) — trade session times
* [SymbolParamsMany.md](./6.%20Additional_Methods/SymbolParamsMany.md) — detailed parameters for many symbols in one call
* [TickValueWithSize.md](./6.%20Additional_Methods/TickValueWithSize.md) — tick value and contract size for P&L calculations

### Bar History

* [PriceHistory.md](./6.%20Additional_Methods/PriceHistory.md) — OHLC bars for a symbol and timeframe

---

## 📡 Subscriptions (Streaming)

* **Overview:** [Streaming_Methods.Overview.md](./7.%20Streaming_Methods/Streaming_Methods.Overview.md)

### Real-Time Price Updates

* [OnSymbolTick.md](./7.%20Streaming_Methods/OnSymbolTick.md) — real-time tick stream for symbols

### Trading Events

* [OnTrade.md](./7.%20Streaming_Methods/OnTrade.md) — position/order changes (opened, closed, modified)
* [OnTradeTransaction.md](./7.%20Streaming_Methods/OnTradeTransaction.md) — detailed transaction log (complete audit trail)

### Position Monitoring

* [OnPositionProfit.md](./7.%20Streaming_Methods/OnPositionProfit.md) — periodic profit/loss updates
* [OnPositionsAndPendingOrdersTickets.md](./7.%20Streaming_Methods/OnPositionsAndPendingOrdersTickets.md) — periodic ticket lists (lightweight)

---

## 🎯 Quick Navigation by Use Case

| I want to... | Use this method |
|-------------|-----------------|
| **ACCOUNT INFORMATION** | |
| Get complete account snapshot | [AccountSummary](./1.%20Account_Information/AccountSummary.md) |
| Get account balance | [AccountInfoDouble](./1.%20Account_Information/AccountInfoDouble.md) (BALANCE) |
| Get account equity | [AccountInfoDouble](./1.%20Account_Information/AccountInfoDouble.md) (EQUITY) |
| Get account leverage | [AccountInfoInteger](./1.%20Account_Information/AccountInfoInteger.md) (LEVERAGE) |
| Get account currency | [AccountInfoString](./1.%20Account_Information/AccountInfoString.md) (CURRENCY) |
| **SYMBOL INFORMATION** | |
| Get current price for symbol | [SymbolInfoTick](./2.%20Symbol_Information/SymbolInfoTick.md) |
| List all available symbols | [SymbolsTotal](./2.%20Symbol_Information/SymbolsTotal.md) + [SymbolName](./2.%20Symbol_Information/SymbolName.md) |
| Add symbol to Market Watch | [SymbolSelect](./2.%20Symbol_Information/SymbolSelect.md) (true) |
| Get symbol digits (decimal places) | [SymbolInfoInteger](./2.%20Symbol_Information/SymbolInfoInteger.md) (DIGITS) |
| Get point size for symbol | [SymbolInfoDouble](./2.%20Symbol_Information/SymbolInfoDouble.md) (POINT) |
| Get complete symbol data (batch) | [SymbolParamsMany](./6.%20Additional_Methods/SymbolParamsMany.md) |
| Get tick value for P&L calculations | [TickValueWithSize](./6.%20Additional_Methods/TickValueWithSize.md) |
| **POSITIONS & ORDERS** | |
| Count open positions | [PositionsTotal](./3.%20Positions_Orders/PositionsTotal.md) |
| Get all open positions (full details) | [OpenedOrders](./3.%20Positions_Orders/OpenedOrders.md) |
| Get position ticket numbers only | [OpenedOrdersTickets](./3.%20Positions_Orders/OpenedOrdersTickets.md) |
| Get historical orders | [OrderHistory](./3.%20Positions_Orders/OrderHistory.md) |
| Get closed positions | [PositionsHistory](./3.%20Positions_Orders/PositionsHistory.md) |
| **MARKET DEPTH** | |
| Subscribe to Level II quotes | [MarketBookAdd](./5.%20Market_Depth(DOM)/MarketBookAdd.md) |
| Get order book data | [MarketBookGet](./5.%20Market_Depth(DOM)/MarketBookGet.md) |
| Unsubscribe from Level II | [MarketBookRelease](./5.%20Market_Depth(DOM)/MarketBookRelease.md) |
| **TRADING OPERATIONS** | |
| Open market BUY position | [OrderSend](./4.%20Trading_Operations/OrderSend.md) (type=BUY) |
| Open market SELL position | [OrderSend](./4.%20Trading_Operations/OrderSend.md) (type=SELL) |
| Place pending orders | [OrderSend](./4.%20Trading_Operations/OrderSend.md) (BUY_LIMIT/SELL_LIMIT/BUY_STOP/SELL_STOP) |
| Modify SL/TP of position | [OrderModify](./4.%20Trading_Operations/OrderModify.md) |
| Close a position | [OrderClose](./4.%20Trading_Operations/OrderClose.md) |
| Calculate margin before trade | [OrderCalcMargin](./4.%20Trading_Operations/OrderCalcMargin.md) |
| Calculate potential profit | [OrderCalcProfit](./4.%20Trading_Operations/OrderCalcProfit.md) |
| Validate trade before execution | [OrderCheck](./4.%20Trading_Operations/OrderCheck.md) |
| **REAL-TIME SUBSCRIPTIONS** | |
| Stream live prices | [OnSymbolTick](./7.%20Streaming_Methods/OnSymbolTick.md) |
| Monitor trade events | [OnTrade](./7.%20Streaming_Methods/OnTrade.md) |
| Track profit changes | [OnPositionProfit](./7.%20Streaming_Methods/OnPositionProfit.md) |
| Monitor ticket changes | [OnPositionsAndPendingOrdersTickets](./7.%20Streaming_Methods/OnPositionsAndPendingOrdersTickets.md) |
| Detailed transaction log | [OnTradeTransaction](./7.%20Streaming_Methods/OnTradeTransaction.md) |

---

## 🏗️ API Architecture

### Layer 1: MT5Account (Low-Level) — You are here!

**What:** direct proto/gRPC communication with the MT5 terminal.

**When to use:**
- You need full control over the protocol
- You are building custom wrappers
- Proto-level integration is required

**Characteristics:**
- Works with proto `*Request` / `*Data` objects
- Raw gRPC method calls
- Complete access to all MT5 functions
- Highest complexity

**Location:** `include/metarpc/mt5_account.hpp` + `src/mt5_account.cpp` (on top of the gRPC stubs generated at build time from `proto/mt5_term_api.desc` into `build/generated/`)

**Documentation:** this folder (you are here!)

---

### Layer 2: MT5Service

**What:** a friendly facade over `MT5Account` — clean arguments, friendly C++ structs, times in UTC. Its header is **protobuf-free**, so strategy code compiles fast.

**When to use:**
- You want a clean API without touching protobuf
- You want named struct fields instead of proto getters
- You want times already converted to UTC

**Location:** `include/metarpc/mt5_service.hpp` + `src/mt5_service.cpp`

**Documentation:** [MT5Service.Overview.md](../MT5Service/MT5Service.Overview.md)

---

### Layer 3: MT5Sugar

**What:** high-level convenience API with ~60 one-liner methods.

**When to use:**
- Most trading scenarios (95% of cases)
- You want risk-based position sizing and smart defaults
- You are building strategies quickly

**Location:** `include/metarpc/mt5_sugar.hpp` + `src/mt5_sugar.cpp`

**Documentation:** [MT5Sugar.API_Overview.md](../MT5Sugar/MT5Sugar.API_Overview.md)

---

## 🎓 Learning Path

**Recommended sequence:** start from the foundation (MT5Account) → build up to the convenience layers (MT5Service → MT5Sugar).

Each layer **owns** the one below it: `MT5Sugar` owns an `MT5Service`, which owns an `MT5Account`. From Sugar you can always reach down with `sugar.service()` and `sugar.account()`.

**Summary:** MT5Account (foundation) → MT5Service (friendly facade) → MT5Sugar (one-liners).

---

## 💡 Key Concepts

### Proto Return Codes

* **10009** = Success / DONE
* **10004** = Requote
* **10006** = Request rejected
* **10013** = Invalid request
* **10014** = Invalid volume
* **10015** = Invalid price
* **10016** = Invalid stops
* **10018** = Market closed
* **10019** = Not enough money
* **10031** = No connection with trade server

Always check the `returned_code` field of a trading reply. See the full [Return Codes Reference](../All_Guides/RETURN_CODES_REFERENCE.md).

---

## ⚠️ Important Notes

* **Demo account first:** always test on demo before live trading.
* **Check return codes:** every trading operation returns a status code (`10009` = success).
* **Validate parameters:** use `orderCheck()` before `orderSend()`.
* **Handle errors:** catch `ApiError` (server rejected the request) and `TransportError` (network/transport failed past retries).
* **Stream cleanup:** call `stop()` on the stream handle, and keep the handle alive while you need events.
* **Time zones:** raw-layer time fields are in the broker server's time zone; the `MT5Service` layer converts them to UTC.
* **Broker limitations:** not all brokers support all features (DOM, hedging, etc.).

---

"Trade safe, code clean, and may your gRPC streams always flow smoothly."
