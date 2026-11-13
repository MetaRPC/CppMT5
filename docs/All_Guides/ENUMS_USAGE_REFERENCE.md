# Enums Reference

A complete catalog of the enums used across CppMT5 — the **friendly C++ enums** you use at the Service/Sugar level, and the **protobuf enums** you meet at the raw `MT5Account` level. They split naturally into **input** enums (you *pass* them, e.g. "which property?", "which order type?") and **output** enums (you *read* them back, e.g. a position's type, a deal's entry, an order's state).

> **🧱 C++ rule — values are namespace-scope.** The generated protobuf enums are *not* `enum class`, so every value lives directly in the `mt5_term_api` namespace: write `pb::ACCOUNT_BALANCE`, `pb::TMT5_ORDER_TYPE_BUY`, `pb::BMT5_POSITION_TYPE_SELL` (where `namespace pb = mt5_term_api;`). The prefixed `pb::SomeEnumType_VALUE` form is a Go/C# convention and does **not** exist in C++.

---

## 1. Friendly C++ enums (`metarpc::…`)

Defined in `mt5_service.hpp`, usable without protobuf. The friendly layers accept/return these; they mirror the raw wire values 1:1.

### `TradeAction` — order direction/type (Service/Sugar input)

```cpp
enum class TradeAction { Buy=0, Sell=1, BuyLimit=2, SellLimit=3,
                         BuyStop=4, SellStop=5, BuyStopLimit=6, SellStopLimit=7 };

OrderRequest q; q.symbol = "EURUSD"; q.action = TradeAction::BuyLimit; q.price = 1.0800;
```

### `Timeframe` — bar period (raw MQL5 `ENUM_TIMEFRAMES` values)

```cpp
enum class Timeframe { M1=1,M2=2,M3=3,M4=4,M5=5,M6=6,M10=10,M12=12,M15=15,M20=20,M30=30,
                       H1=16385,H2=16386,H3=16387,H4=16388,H6=16390,H8=16392,H12=16396,
                       D1=16408,W1=32769,MN1=49153 };

auto bars = svc.lastBars("EURUSD", Timeframe::H1, 100);
```

### `TradeMode` — account type (Service output, mirrors `MrpcEnumAccountTradeMode`)

```cpp
enum class TradeMode { Demo=0, Contest=1, Real=2 };
if (svc.accountSummary().tradeMode == TradeMode::Demo) { /* … */ }
```

---

## 2. Input enums (you pass these)

### 2.1 Account property selectors

Used by `accountInfoDouble/Integer/String` (both layers). At the Service level you pass the id as a plain `int`.

**`AccountInfoDoublePropertyType`** (14)

| id | Constant | id | Constant |
|----|----------|----|----------|
| 0 | `ACCOUNT_BALANCE` | 7 | `ACCOUNT_MARGIN_SO_CALL` |
| 1 | `ACCOUNT_CREDIT` | 8 | `ACCOUNT_MARGIN_SO_SO` |
| 2 | `ACCOUNT_PROFIT` | 9 | `ACCOUNT_MARGIN_INITIAL` |
| 3 | `ACCOUNT_EQUITY` | 10 | `ACCOUNT_MARGIN_MAINTENANCE` |
| 4 | `ACCOUNT_MARGIN` | 11 | `ACCOUNT_ASSETS` |
| 5 | `ACCOUNT_MARGIN_FREE` | 12 | `ACCOUNT_LIABILITIES` |
| 6 | `ACCOUNT_MARGIN_LEVEL` | 13 | `ACCOUNT_COMMISSION_BLOCKED` |

**`AccountInfoIntegerPropertyType`** (11): `ACCOUNT_LOGIN`(0), `ACCOUNT_TRADE_MODE`(1), `ACCOUNT_LEVERAGE`(2), `ACCOUNT_LIMIT_ORDERS`(3), `ACCOUNT_MARGIN_SO_MODE`(4), `ACCOUNT_TRADE_ALLOWED`(5), `ACCOUNT_TRADE_EXPERT`(6), `ACCOUNT_MARGIN_MODE`(7), `ACCOUNT_CURRENCY_DIGITS`(8), `ACCOUNT_FIFO_CLOSE`(9), `ACCOUNT_HEDGE_ALLOWED`(10).

**`AccountInfoStringPropertyType`** (4): `ACCOUNT_NAME`(0), `ACCOUNT_SERVER`(1), `ACCOUNT_CURRENCY`(2), `ACCOUNT_COMPANY`(3).

### 2.2 Symbol property selectors

Used by `symbolInfoDouble/Integer/String`.

**`SymbolInfoDoubleProperty`** (60 values) — common: `SYMBOL_BID`(0), `SYMBOL_ASK`(3), `SYMBOL_LAST`(6), `SYMBOL_POINT`(13), `SYMBOL_TRADE_TICK_VALUE`(14), `SYMBOL_TRADE_TICK_SIZE`(17), `SYMBOL_TRADE_CONTRACT_SIZE`(18), `SYMBOL_VOLUME_MIN`(22), `SYMBOL_VOLUME_MAX`(23), `SYMBOL_VOLUME_STEP`(24), `SYMBOL_SWAP_LONG`(26), `SYMBOL_SWAP_SHORT`(27), `SYMBOL_MARGIN_INITIAL`(35).

**`SymbolInfoIntegerProperty`** — common: `SYMBOL_EXIST`(6), `SYMBOL_SELECT`(7), `SYMBOL_TIME`(15), `SYMBOL_DIGITS`(17), `SYMBOL_SPREAD`(19), `SYMBOL_TRADE_MODE`(22), `SYMBOL_TRADE_STOPS_LEVEL`(25), `SYMBOL_FILLING_MODE`(32).

**`SymbolInfoStringProperty`** — common: `SYMBOL_CURRENCY_BASE`(5), `SYMBOL_CURRENCY_PROFIT`(6), `SYMBOL_CURRENCY_MARGIN`(7), `SYMBOL_DESCRIPTION`(9), `SYMBOL_PATH`(14).

> There are too many symbol properties to list in full here — dump the complete set with the [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md) (`enum SymbolInfoDoubleProperty`), or just use `MT5Service::symbolParams()` which returns a named `SymbolInfo` with every field spelled out.

### 2.3 Order-type enums (three parallel sets, same values 0…8)

| Enum | Where it's the input | Values |
|------|----------------------|--------|
| `TMT5_ENUM_ORDER_TYPE` | `OrderSendRequest.operation` | `TMT5_ORDER_TYPE_BUY`(0), `..._SELL`(1), `..._BUY_LIMIT`(2), `..._SELL_LIMIT`(3), `..._BUY_STOP`(4), `..._SELL_STOP`(5), `..._BUY_STOP_LIMIT`(6), `..._SELL_STOP_LIMIT`(7), `..._CLOSE_BY`(8) |
| `ENUM_ORDER_TYPE_TF` | `OrderCalcMargin/Profit`, `OrderCheck`'s `order_type` | `ORDER_TYPE_TF_BUY`(0) … `ORDER_TYPE_TF_CLOSE_BY`(8) |
| `ENUM_ORDER_TYPE` | `SymbolInfoMarginRate`'s `order_type` | `ORDER_TYPE_BUY`(0) … `ORDER_TYPE_CLOSE_BY`(8) |

The friendly `TradeAction` lines up 1:1 with all three, so `(int)TradeAction::Buy == 0`.

### 2.4 Trade-request shaping (raw `OrderCheck` / `MqlTradeRequest`)

| Enum | Field | Values |
|------|-------|--------|
| `MRPC_ENUM_TRADE_REQUEST_ACTIONS` | `action` | `TRADE_ACTION_DEAL`(0), `TRADE_ACTION_PENDING`(1), `TRADE_ACTION_SLTP`(2), `TRADE_ACTION_MODIFY`(3), `TRADE_ACTION_REMOVE`(4), `TRADE_ACTION_CLOSE_BY`(5) |
| `MRPC_ENUM_ORDER_TYPE_FILLING` | `type_filling` | `ORDER_FILLING_FOK`(0), `ORDER_FILLING_IOC`(1), `ORDER_FILLING_RETURN`(2), `ORDER_FILLING_BOC`(3) |
| `TMT5_ENUM_ORDER_TYPE_TIME` | `expiration_time_type` / `type_time` | `TMT5_ORDER_TIME_GTC`(0), `TMT5_ORDER_TIME_DAY`(1), `TMT5_ORDER_TIME_SPECIFIED`(2), `TMT5_ORDER_TIME_SPECIFIED_DAY`(3) |

### 2.5 Day of week & sort selectors

| Enum | Where | Values |
|------|-------|--------|
| `DayOfWeek` | `SymbolInfoSessionQuote/Trade` | `SUNDAY`(0), `MONDAY`(1), `TUESDAY`(2), `WEDNESDAY`(3), `THURSDAY`(4), `FRIDAY`(5), `SATURDAY`(6) |
| `BMT5_ENUM_ORDER_HISTORY_SORT_TYPE` | `OrderHistory.inputSortMode` | `BMT5_SORT_BY_OPEN_TIME_ASC`(0), `..._OPEN_TIME_DESC`(1), `..._CLOSE_TIME_ASC`(2), `..._CLOSE_TIME_DESC`(3), `..._ORDER_TICKET_ID_ASC`(4), `..._ORDER_TICKET_ID_DESC`(5) |
| `AH_ENUM_POSITIONS_HISTORY_SORT_TYPE` | `PositionsHistory.sort_type` | `AH_POSITION_OPEN_TIME_ASC`(0), `..._OPEN_TIME_DESC`(1), `..._TICKET_ASC`(2), `..._TICKET_DESC`(3) |
| `BMT5_ENUM_OPENED_ORDER_SORT_TYPE` | `OpenedOrders.inputSortMode` | sort order for the open positions/orders lists |

---

## 3. Output enums (you read these back)

### 3.1 Account state (in `AccountSummaryData`)

| Enum | Field | Values |
|------|-------|--------|
| `MrpcEnumAccountTradeMode` | `account_trade_mode()` | `MRPC_ACCOUNT_TRADE_MODE_DEMO`(0), `..._CONTEST`(1), `..._REAL`(2) |
| `MrpcEnumAccountMarginMode` | `account_margin_mode()` | `MRPC_ACCOUNT_MARGIN_MODE_RETAIL_NETTING`(0), `..._EXCHANGE`(1), `..._RETAIL_HEDGING`(2) |

### 3.2 Positions, orders & deals (in the data payloads)

| Enum | Appears on | Values |
|------|-----------|--------|
| `BMT5_ENUM_POSITION_TYPE` | `PositionInfo.type` | `BMT5_POSITION_TYPE_BUY`(0), `BMT5_POSITION_TYPE_SELL`(1) |
| `BMT5_ENUM_POSITION_REASON` | `PositionInfo.reason` | `..._CLIENT`(0), `..._MOBILE`(1), `..._WEB`(2), `..._EXPERT`(3), `ORDER_REASON_SL`(4), `ORDER_REASON_TP`(5), `ORDER_REASON_SO`(6) |
| `BMT5_ENUM_ORDER_STATE` | order `state` | `..._STARTED`(0), `..._PLACED`(1), `..._CANCELED`(2), `..._PARTIAL`(3), `..._FILLED`(4), `..._REJECTED`(5), `..._EXPIRED`(6), `..._REQUEST_ADD`(7), `..._REQUEST_MODIFY`(8), `..._REQUEST_CANCEL`(9) |
| `BMT5_ENUM_DEAL_TYPE` | `DealHistoryData.type` | `..._BUY`(0), `..._SELL`(1), `..._BALANCE`(2), `..._CREDIT`(3), `..._CHARGE`(4), `..._CORRECTION`(5), `..._BONUS`(6), `..._COMMISSION`(7) … (and more) |
| `BMT5_ENUM_DEAL_ENTRY_TYPE` | deal `entry` | `BMT5_DEAL_ENTRY_IN`(0), `..._OUT`(1), `..._INOUT`(2), `..._OUT_BY`(3) |

### 3.3 Status / result enums

| Enum | Appears on | Values |
|------|-----------|--------|
| `MRPC_ORDER_CLOSE_MODE` | `OrderCloseData.close_mode()` | `MRPC_MARKET_ORDER_CLOSE`(0), `MRPC_MARKET_ORDER_PARTIAL_CLOSE`(1), `MRPC_PENDING_ORDER_REMOVE`(2) |
| `BookType` | `MrpcMqlBookInfo.type()` (DOM) | `BOOK_TYPE_SELL`(0), `BOOK_TYPE_BUY`(1), `BOOK_TYPE_SELL_MARKET`(2), `BOOK_TYPE_BUY_MARKET`(3) |
| `MT5_SUB_ENUM_EVENT_GROUP_TYPE` | every `On*` streaming payload's `type()` | `OrderProfit`(0), `OrderUpdate`(1) |

> The **trade retcode** (`returned_code`, `10009` = DONE) is *not* an enum — it's a `uint32`. See [Return Codes](RETURN_CODES_REFERENCE.md).

---

## 4. Usage at each layer

```cpp
namespace pb = mt5_term_api;

// Low level — the real protobuf enum, namespace-scope value:
pb::AccountInfoDoubleRequest r;
r.set_property_id(pb::ACCOUNT_MARGIN_FREE);              // input enum
double fm = acc.accountInfoDouble(r).requested_value();  // account → requested_value(); symbol → value()

// Reading an output enum:
pb::AccountSummaryData s = acc.accountSummary();
if (s.account_trade_mode() == pb::MRPC_ACCOUNT_TRADE_MODE_REAL) { /* be careful */ }

// Mid level — pass the id as a plain int (the enum value converts implicitly):
double freeMargin = svc.accountInfoDouble(pb::ACCOUNT_MARGIN_FREE);
int64_t digits    = svc.symbolInfoInteger("EURUSD", pb::SYMBOL_DIGITS);
```

!!! tip
    You rarely need the raw property ids — `MT5Service::symbolParams()` returns a `SymbolInfo` with
    every field already named (`digits`, `point`, `volumeMin`, `spread`, …), and `accountSummary()`
    returns a named `AccountInfo`. Reach for property ids only when you want a single value the
    structs don't carry.

---

## 5. Discovering enum values

Don't guess the constants — list them from the real descriptors with the [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md):

```text
> enum TMT5_ENUM_ORDER_TYPE
  TMT5_ORDER_TYPE_BUY        = 0
  TMT5_ORDER_TYPE_SELL       = 1
  ...
> enum BMT5_ENUM_DEAL_TYPE
> field property_id           # which messages take a property enum
```

It runs offline (`./build/protobuf_inspector`) and prints the exact names and values compiled into the SDK.

---

## 📚 See also

* [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md) — browse every message, field and enum
* [Return Codes](RETURN_CODES_REFERENCE.md) — the trade retcodes (not enums)
* Reference: [MT5Account](../MT5Account/MT5Account.Master.Overview.md) · [MT5Service](../MT5Service/MT5Service.Overview.md)
