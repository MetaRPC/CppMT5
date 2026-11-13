# 📖 CppMT5 Glossary

> The terms and concepts used across the CppMT5 SDK and MetaTrader 5 trading — the three layers in depth, then the C++, protocol, and trading vocabulary.

---

## 🏗️ The three layers

### MT5Account

**🔹 Layer 1 — Low-level API**

The base layer: direct access to the MT5 terminal over gRPC.

**Key characteristics**

- Builds each protobuf `…Request` and attaches `apikey` + session `id` metadata.
- Unwraps every reply's `oneof { data, error }` into the typed `…Data` payload, or **throws** `ApiError`.
- Returns the protobuf `…Data` payloads — full control, maximum detail.
- Owns the channel, stubs, retry/reconnect, and streaming (`onSymbolTick`, `onTrade`, …).
- **39 methods** across 7 groups, plus connection/identity helpers.

**When to use:** you need a field or variant the higher layers don't surface, or raw protobuf control.

**Location:** `include/metarpc/mt5_account.hpp` + `src/mt5_account.cpp`

**Docs:** [MT5Account Master Overview](../MT5Account/MT5Account.Master.Overview.md)

---

### MT5Service

**🔸 Layer 2 — Friendly facade**

The middle layer: friendly C++ structs and plain arguments, times in UTC — no protobuf.

**Key characteristics**

- Takes plain args (`&str` symbols, `int` property ids, typed enums); builds the requests for you.
- Unwraps `…Data` into clean values and **named structs** (`AccountInfo`, `Tick`, `SymbolInfo`, `OrderResult`, …).
- Converts broker-server time ↔ **UTC** automatically (`utcTimes = true` by default).
- The header is **protobuf-free** (forward-declares `MT5Account`) → fast compiles.
- **~58 methods** across 6 groups; reach the layer below with `account()`.

**When to use:** most application code — ergonomic, typed calls without protobuf ceremony.

**Location:** `include/metarpc/mt5_service.hpp` + `src/mt5_service.cpp`

**Docs:** [MT5Service Overview](../MT5Service/MT5Service.Overview.md)

---

### MT5Sugar

**🔺 Layer 3 — Convenience API**

The high-level layer: business logic and one-liners.

**Key characteristics**

- Order construction from side/volume; pip-based SL/TP; risk-based position sizing.
- Compact result structs (`PriceInfo`, `DailyStats`) on top of the Service structs.
- Order methods return the **resulting ticket** (`0` on rejection); management methods return `bool` / counts.
- Also protobuf-free; reach down with `service()` and `account()`.
- **~58 methods** across 11 groups — the best starting point for most apps.

**Key examples**

- `buyMarket(symbol, volume)` — open a position in one line.
- `buyMarketWithPips(symbol, volume, slPips, tpPips)` — open with a pip bracket.
- `calculatePositionSize(symbol, riskPercent, slPips)` — size by risk.
- `getBalance()` / `getEquity()` · `closeAllPositions()` — quick reads and an emergency exit.

**When to use:** rapid development, prototyping, simple strategies.

**Location:** `include/metarpc/mt5_sugar.hpp` + `src/mt5_sugar.cpp`

**Docs:** [MT5Sugar API Overview](../MT5Sugar/MT5Sugar.API_Overview.md)

---

## ⚙️ C++ & concurrency concepts

### No async/await — blocking calls + threads

Every unary method is an ordinary **blocking** function that returns a value or **throws**. There is no async runtime. Run work concurrently with `std::thread` / `std::async`. See [Concurrency Model](SYNC_VS_ASYNC.md).

### Errors are exceptions

Calls don't return error codes — they throw. **`ApiError`** = the gateway/terminal rejected the request (bad login, invalid stops, trade rejected…). **`TransportError`** = the gRPC transport failed past the retry budget. Catch them at a sensible boundary.

### Streams

Subscriptions return a `std::unique_ptr<AccountStream>` (low level) or `ServiceStream` (friendly). The SDK reads the server stream on a **background thread** and invokes your callback per payload. Keep the handle alive; `stop()` ends it. Handle API: `stop()` / `running()` / `count()` / `lastError()`. See [gRPC Streaming](GRPC_STREAM_MANAGEMENT.md).

### Friendly structs

`MT5Service` / `MT5Sugar` return plain structs instead of protobuf: **`AccountInfo`** (login, balance, equity, leverage, currency, times…), **`Tick`** (symbol, bid, ask, volume, UTC time), **`SymbolInfo`** (digits, point, volume min/step/max, tick value…), **`OrderResult`** / **`OrderCheckResult`**, **`Position`** / **`PendingOrder`**, **`Bar`**, **`DailyStats`**.

### Enums

Protobuf enums are `int` under the hood. In C++ the generated values live at **namespace scope** — write `pb::ACCOUNT_BALANCE`, `pb::TMT5_ORDER_TYPE_BUY` (not a prefixed form). The friendly layers add typed C++ enums (`TradeAction`, `Timeframe`, `TradeMode`). See [Enums Reference](ENUMS_USAGE_REFERENCE.md).

### `ClientOptions`

Per-client transport knobs: `unaryTimeoutMs`, `connectTimeoutMs`, `tradeTimeoutMs`, `maxRetries`, `maxBackoffMs`, `maxReceiveMessageBytes`, `rootCertsPath`, `verbose`. Reach them via `options()`.

---

## 🔌 gRPC & protocol

- **gRPC** — the RPC protocol the SDK uses to reach the MetaRPC gateway (HTTP/2 + TLS).
- **grpc++ / protobuf** — the C++ libraries the SDK links against (from your toolchain, via pkg-config).
- **`proto/mt5_term_api.desc` (`mt5_term_api`)** — the gRPC contract as a version-independent `FileDescriptorSet`. CMake runs `protoc` on it at build time to generate the C++ + gRPC stubs (namespace `mt5_term_api`, aliased **`pb`** inside the SDK) into `build/generated/`, matching your installed protobuf.
- **`…Request` / `…Data` / `…Reply`** — a call sends a `…Request`; the server answers a `…Reply` carrying a `…Data` payload or an `Error`.
- **oneof `{ data, error }`** — how most replies wrap the result; `MT5Account` unwraps it to `…Data` or throws `ApiError`.
- **apikey** — your MetaRPC token (`""` → `MRPC_API_KEY` env, else `"TRIAL"`), sent as metadata on every call.
- **session id / Terminal GUID (`terminalInstanceGuid`)** — the terminal session the gateway assigns on `connect()` (ConnectEx), auto-attached to later calls; read via `terminalGuid()`.
- **ConnectEx** — the RPC that starts/attaches an MT5 terminal for your account; `connect()` calls it.
- **RetCode (`returned_code`)** — the trade-server result after a trading call; **`10009` = DONE (success)**. See [Return Codes](RETURN_CODES_REFERENCE.md).

---

## 💹 Market & prices

| Term | Meaning |
| --- | --- |
| **Symbol** | A tradable instrument (`EURUSD`); names are broker-specific (suffixes like `.m`). |
| **Bid / Ask** | Best sell / best buy price. **Spread** = ask − bid. |
| **Point** | Smallest price step (`SYMBOL_POINT`), e.g. `0.00001` for 5-digit EURUSD. |
| **Pip** | `10 × point` on 3/5-digit symbols (EURUSD 1 pip = `0.0001`). |
| **Digits** | Decimal places in the price (`SYMBOL_DIGITS`). |
| **Tick** | One price update; **tick value** = money per tick per lot. |
| **Contract size** | Units per lot (e.g. 100,000 for FX). |
| **Volume / lot** | Trade size; bounded by `volumeMin` / `volumeStep` / `volumeMax`. |
| **Bar** | One OHLC candle for a `Timeframe` (M1…MN1). |
| **Market Watch** | The terminal's selected-symbols list; some calls need a symbol selected (`symbolSelect`). |
| **DOM (Depth of Market)** | Level-II order book; subscribe/read/release (`marketBookAdd`/`Get`/`Release`). |

---

## 📈 Orders, positions & deals

In MT5, **orders**, **positions** and **deals** are distinct:

- **Order** — an instruction to trade (market or pending).
- **Position** — the open exposure from a filled order.
- **Deal** — an executed transaction in history; realized P/L lives on deals.
- **Pending order** — rests until a price triggers it: **Limit** (better price) / **Stop** (breakout) / **Stop-Limit**.
- **SL / TP** — protective Stop-Loss / Take-Profit exit prices.
- **Stops level** (`SYMBOL_TRADE_STOPS_LEVEL`) — minimum distance SL/TP may sit from the market, in points.
- **Freeze level** — distance within which modify/close is blocked.
- **Slippage / deviation** — allowed price movement between request and fill, in points.
- **Magic number** — a client id tagging orders from one strategy (sent as `expert_id`).
- **Ticket** — the numeric id of an order / position / deal.

---

## 💰 Account & money

| Term | Meaning |
| --- | --- |
| **Balance** | Realized funds, excluding floating P/L. |
| **Equity** | Balance + floating P/L (live account value). |
| **Margin** | Funds reserved to hold open positions. |
| **Free margin** | `equity − margin`; available for new positions. |
| **Margin level** | `equity / margin × 100`, in percent (margin-call / stop-out below thresholds; `0` when flat). |
| **Leverage** | Borrowing ratio (e.g. 1:100). |
| **Floating / unrealized P/L** | Profit of still-open positions (`equity − balance`). |
| **Realized P/L** | Profit of closed positions (from history). |

---

## 🕐 Time

- **Broker server time** — the raw time on the wire; `MT5Account` returns it unchanged.
- **UTC conversion** — `MT5Service` / `MT5Sugar` convert to/from UTC using the broker offset (`serverOffsetSeconds`). With `utcTimes = true` (the default) every time you see and pass is **UTC**.

---

## 🛡️ Strategy mechanics

- **Breakeven** — moving SL to (around) the entry once in profit, to remove risk.
- **Trailing stop** — an SL that follows price in the favorable direction, ratcheting and never loosening.
- **Risk sizing** — choosing lot size so a stop-out risks a fixed % of balance (`calculatePositionSize`).
- **Orchestrator** — a small strategy manager (loop: read → act → repeat) built on `MT5Sugar`: [Trailing Stop](../Strategies/Orchestrators/TrailingStop.md), [Grid Trader](../Strategies/Orchestrators/GridTrader.md), [Risk Manager](../Strategies/Orchestrators/RiskManager.md).
- **Preset** — a coordinator that classifies the market and dispatches to an orchestrator: [Adaptive Market Mode](../Strategies/Presets/AdaptiveMarketModePreset.md). See [Strategies](../Strategies/Strategies.Master.Overview.md).

---

## 📚 See also

- [Getting Started](GETTING_STARTED.md) · [Project Map](PROJECT_MAP.md) · [MT5 for Beginners](MT5_For_Beginners.md)
- [Return Codes](RETURN_CODES_REFERENCE.md) · [Enums Reference](ENUMS_USAGE_REFERENCE.md) · [gRPC Streaming](GRPC_STREAM_MANAGEMENT.md) · [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md)
