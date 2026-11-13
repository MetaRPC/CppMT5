# MT5Service — Mid-Level API Overview

> A comfortable, **protobuf-free** facade over MT5Account: friendly C++ structs instead of protobuf messages, plain arguments instead of hand-built `*Request` objects, and automatic UTC ↔ broker-server time conversion.

**🧩 API Layer:** **MID-LEVEL** — the balance between low-level ([MT5Account](../MT5Account/MT5Account.Master.Overview.md)) and high-level ([MT5Sugar](../MT5Sugar/MT5Sugar.API_Overview.md)). **Recommended for most application code.**

---

## ⚠️ What changes vs MT5Account

`MT5Service` and `MT5Account` have the **same underlying gRPC calls** — the difference is the **types** and the **ceremony**:

```cpp
// Low-level (MT5Account): build a request, unpack the Data wrapper
namespace pb = mt5_term_api;
pb::AccountInfoDoubleRequest r;
r.set_property_id(pb::ACCOUNT_BALANCE);
double balance = account.accountInfoDouble(r).requested_value();   // ← unpack

// Mid-level (MT5Service): one call, plain value
double balance = service.accountInfoDouble(pb::ACCOUNT_BALANCE);   // ← done
```

```cpp
// Low-level: protobuf tick, manual time handling
pb::SymbolInfoTickRequest tr; tr.set_symbol("EURUSD");
pb::MrpcMqlTick t = account.symbolInfoTick(tr);
// t.time() is raw broker server time

// Mid-level: friendly struct, time already UTC
Tick tick = service.symbolInfoTick("EURUSD");
std::printf("%.5f / %.5f\n", tick.bid, tick.ask);   // tick.time is UTC seconds
```

**The header is protobuf-free.** `#include <metarpc/mt5_service.hpp>` does **not** pull the ~11 MB generated protobuf headers (it forward-declares `MT5Account`), so strategy code compiles fast and only sees the friendly types. Reach the raw layer with `service.account()` when you need an RPC not surfaced here.

---

## 🏗️ Architecture

```
┌───────────────────────────────────────────────┐
│  HIGH-LEVEL: MT5Sugar        ← one-liners      │
└───────────────────────────────────────────────┘
                    ↓ owns
┌───────────────────────────────────────────────┐
│  MID-LEVEL: MT5Service  ← YOU ARE HERE         │
│  friendly C++ structs, UTC times               │
└───────────────────────────────────────────────┘
                    ↓ owns
┌───────────────────────────────────────────────┐
│  LOW-LEVEL: MT5Account   ← protobuf / gRPC     │
└───────────────────────────────────────────────┘
```

Each layer **owns** the one below: `MT5Service` owns an `MT5Account`. Construct the service directly — it creates the account for you.

---

## 📦 Creating & connecting

```cpp
#include <metarpc/mt5_service.hpp>
using namespace metarpc;

// host, port, apiKey, name, utcTimes(=true by default)
MT5Service service("mt5.mrpc.pro", 443);

service.connect(5057947437, "password", "MetaQuotes-Demo");   // blocks until ready
// ... use friendly methods ...
service.disconnect();   // stops the terminal on the server; or close() to just drop the channel
```

* **No `ctx` parameter.** Deadlines come from `service.options()` (a `ClientOptions&`): `unaryTimeoutMs`, `tradeTimeoutMs`, `maxRetries`, …
* **Errors are exceptions:** `ApiError` (server rejected) / `TransportError` (transport failed past retries).
* **`utcTimes = true`** (default): every time the facade returns is **UTC seconds**, and every time you pass in is treated as UTC. Set `false` in the constructor to work in raw broker-server time.
* **Reach low-level:** `MT5Account& acc = service.account();`

---

## 📚 All methods by category

### 1️⃣ Account Methods (4) → [full docs](./1.%20Account_Methods.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `accountSummary()` | `AccountInfo` | **Recommended** — all account info at once |
| `accountInfoDouble(propertyId)` | `double` | One double property (balance, equity, margin) |
| `accountInfoInteger(propertyId)` | `int64_t` | One integer property (login, leverage) |
| `accountInfoString(propertyId)` | `std::string` | One string property (currency, company) |

### 2️⃣ Symbol Methods (16) → [full docs](./2.%20Symbol_Methods.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `symbolsTotal(selectedOnly=false)` | `int` | Count of symbols |
| `symbolExist(symbol)` | `bool` | Does the broker know it |
| `symbolName(index, selectedOnly=false)` | `std::string` | Name by index |
| `symbolSelect(symbol, select=true)` | `bool` | Add/remove in Market Watch |
| `symbolIsSynchronized(symbol)` | `bool` | Is its data synced |
| `symbolList()` | `std::vector<std::string>` | All symbol names |
| `symbolInfoTick(symbol)` | `Tick` | Latest tick (UTC time) |
| `symbolParams(symbol)` | `SymbolInfo` | **Recommended** — full spec for one symbol |
| `symbolParamsMany(nameFilter="")` | `std::vector<SymbolInfo>` | Full spec for many symbols |
| `symbolInfoDouble/Integer/String(symbol, propertyId)` | `double`/`int64_t`/`string` | One property |
| `symbolInfoMarginRate(symbol, orderType)` | `MarginRate` | Margin rates |
| `symbolInfoSessionQuote(symbol, day, idx)` | `SessionTime` | Quote session window |
| `symbolInfoSessionTrade(symbol, day, idx)` | `SessionTime` | Trade session window |
| `tickValueWithSize(symbols)` | `std::vector<TickValueSize>` | Tick value/size (P&L) |
| `priceHistory(symbol, tf, from, to, maxBars=0)` | `std::vector<Bar>` | OHLC bars in a window |
| `lastBars(symbol, tf, count)` | `std::vector<Bar>` | Newest `count` bars |

### 3️⃣ Position & Orders Methods (6) → [full docs](./3.%20Position_Orders_Methods.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `positionsTotal()` | `int` | Count of open positions |
| `openedOrders()` | `OpenedOrders` | Positions + pending orders (friendly) |
| `positions(magic=-1)` | `std::vector<Position>` | Open positions (optionally by magic) |
| `openedOrdersTickets()` | `OpenedTickets` | **Lightweight** — ticket numbers only |
| `orderHistory(from, to)` | `std::vector<HistoryItem>` | Orders/deals history (UTC window) |
| `positionsHistory(from, to)` | `std::vector<PositionHistory>` | Closed positions (UTC window) |

### 4️⃣ Trading Methods (6) → [full docs](./4.%20Trading_Methods.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `orderSend(OrderRequest)` | `OrderResult` | Send market/pending order |
| `orderModify(ticket, sl, tp, price=0, stopLimit=0)` | `OrderResult` | Modify SL/TP (and pending price) |
| `orderClose(ticket, volume=0, slippage=10)` | `OrderResult` | Close/partial-close or delete pending |
| `orderCheck(OrderRequest)` | `OrderCheckResult` | Dry-run validate before sending |
| `orderCalcMargin(symbol, orderType, volume, price)` | `double` | Required margin |
| `orderCalcProfit(symbol, orderType, volume, open, close)` | `double` | Potential profit |

### 5️⃣ Market Depth Methods (3) → [full docs](./5.%20MarketDepth_Methods.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `marketBookAdd(symbol)` | `bool` | Subscribe to DOM |
| `marketBookGet(symbol)` | `std::vector<BookEntry>` | Current DOM snapshot |
| `marketBookRelease(symbol)` | `bool` | Unsubscribe |

### 6️⃣ Streaming Methods (5) → [full docs](./6.%20Streaming_Methods.md)

| Method | Returns | Description |
|--------|---------|-------------|
| `subscribeTicks(symbols, cb)` | `std::unique_ptr<ServiceStream>` | Live `Tick`s (UTC) |
| `streamTradeUpdates(cb)` | `std::unique_ptr<ServiceStream>` | `TradeEvent`s |
| `streamTransactions(cb)` | `std::unique_ptr<ServiceStream>` | `TransactionEvent`s |
| `streamPositionProfits(cb)` | `std::unique_ptr<ServiceStream>` | `PositionProfitEvent`s |
| `streamOpenedTickets(cb)` | `std::unique_ptr<ServiceStream>` | `TicketsSnapshot`s |

> **Connection/Time helpers** also live on the service: `connect/reconnect/isConnected/terminalGuid/disconnect/close/options/account` and `serverOffsetSeconds/toUtc/toServerTime`.

---

## 📦 Key DTO structs

```cpp
struct AccountInfo {               // accountSummary()
    int64_t login; std::string currency;
    double balance, equity, credit; int64_t leverage;
    std::string name, company; TradeMode tradeMode;
    int64_t serverTime, serverTimeUtc, utcShiftMinutes;  // unix seconds
    std::string server;
};

struct Tick {                      // symbolInfoTick(), subscribeTicks()
    std::string symbol; int64_t time, timeMsc;           // UTC seconds / ms
    double bid, ask, last, volume; uint32_t flags;
    bool twoSided() const;
};

struct OrderResult {               // orderSend/Modify/Close
    uint32_t retcode;              // 10009 DONE, 10008 PLACED, …
    int64_t ticket, deal, order; double volume, price, bid, ask;
    std::string comment, retcodeName, retcodeDescription; uint32_t requestId;
    bool ok() const;               // true for accepted retcodes
};

struct OrderCheckResult {          // orderCheck
    uint32_t retcode;              // 0 / 10009 = would succeed
    double balance, equity, profit, margin, freeMargin, marginLevel;
    std::string comment; bool ok() const;
};
```

Other DTOs: `SymbolInfo`, `Bar`, `Position`, `PendingOrder`, `OpenedOrders`, `Deal`, `HistoryOrder`, `HistoryItem`, `PositionHistory`, `MarginRate`, `SessionTime`, `BookEntry`, `TickValueSize`, `OpenedTickets`, and the streaming event structs (`TradeEvent`, `TransactionEvent`, `PositionProfitEvent`, `TicketsSnapshot`, …). All are declared in `mt5_service.hpp`.

---

## 💡 Usage Examples

### 1) Balance & equity

```cpp
AccountInfo s = service.accountSummary();
std::printf("Balance %.2f %s | Equity %.2f | Lev 1:%lld\n",
            s.balance, s.currency.c_str(), s.equity, (long long)s.leverage);
```

### 2) Current price

```cpp
Tick t = service.symbolInfoTick("EURUSD");
std::printf("%.5f / %.5f (spread %.5f)\n", t.bid, t.ask, t.ask - t.bid);
```

### 3) Validate, then place

```cpp
OrderRequest req;
req.symbol = "EURUSD";
req.action = TradeAction::Buy;
req.volume = 0.10;
req.price  = service.symbolInfoTick("EURUSD").ask;

OrderCheckResult chk = service.orderCheck(req);
if (!chk.ok()) { std::printf("invalid: %s\n", chk.comment.c_str()); return; }
std::printf("margin %.2f, free after %.2f\n", chk.margin, chk.freeMargin);

OrderResult res = service.orderSend(req);
if (res.ok()) std::printf("order #%lld @ %.5f\n", (long long)res.order, res.price);
else          std::printf("rejected: %u %s\n", res.retcode, res.retcodeDescription.c_str());
```

### 4) Live prices (stream)

```cpp
auto stream = service.subscribeTicks({"EURUSD","GBPUSD"}, [](const Tick& t) {
    std::printf("%s %.5f/%.5f\n", t.symbol.c_str(), t.bid, t.ask);
});
// ... keep `stream` alive ...
stream->stop();
```

---

## 📊 Low-level vs Mid-level

| Aspect | MT5Account (low) | MT5Service (mid) |
|--------|------------------|------------------|
| Return type | protobuf `*Data` | friendly C++ structs |
| Request creation | manual `*Request` | automatic |
| Data unpacking | `.requested_value()`, `.total()` | direct |
| Time | raw broker seconds | UTC (by default) |
| Header weight | pulls protobuf | **protobuf-free** |
| Recommended for | specific/raw needs | **most tasks** |

---

## 🔧 Best practices

1. **Prefer the "recommended" aggregate methods.** `accountSummary()` over several `accountInfo*`; `symbolParams()` over many `symbolInfo*`.
2. **Check trade retcodes.** `OrderResult::ok()` / `retcode == 10009`; `OrderCheckResult::ok()`.
3. **Tune timeouts** on `service.options()` instead of a per-call `ctx`.
4. **Clean up streams and DOM.** Keep stream handles alive; `marketBookRelease()` what you `marketBookAdd()`.
5. **Catch exceptions** (`ApiError` / `TransportError`) at a sensible boundary.

---

## 📚 Related

* **Low-level:** [MT5Account Master Overview](../MT5Account/MT5Account.Master.Overview.md)
* **High-level:** [MT5Sugar API Overview](../MT5Sugar/MT5Sugar.API_Overview.md)
* **Category docs:** [Account](./1.%20Account_Methods.md) · [Symbol](./2.%20Symbol_Methods.md) · [Position & Orders](./3.%20Position_Orders_Methods.md) · [Trading](./4.%20Trading_Methods.md) · [Market Depth](./5.%20MarketDepth_Methods.md) · [Streaming](./6.%20Streaming_Methods.md)
