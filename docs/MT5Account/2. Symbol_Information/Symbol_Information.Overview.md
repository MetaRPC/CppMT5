# MT5Account · Symbol Information — Overview

> Quotes, symbol properties, trading rules, and Market Watch management. Use this page to choose the right API for everything symbol-related.

## 📁 What lives here

### Current quotes

* **[SymbolInfoTick](./SymbolInfoTick.md)** — the **current tick** (bid, ask, last, volume, timestamps) in one call. Returns `MrpcMqlTick` directly.

### Symbol inventory & management

* **[SymbolsTotal](./SymbolsTotal.md)** — count of symbols (all, or only Market Watch).
* **[SymbolName](./SymbolName.md)** — resolve an index → symbol name.
* **[SymbolList](./SymbolList.md)** — all symbol names in one call (CppMT5 convenience).
* **[SymbolSelect](./SymbolSelect.md)** — add/remove a symbol in Market Watch.
* **[SymbolExist](./SymbolExist.md)** — does a symbol exist (and is it custom)?
* **[SymbolIsSynchronized](./SymbolIsSynchronized.md)** — is the symbol's data ready?

### Symbol properties (enum-selected)

* **[SymbolInfoDouble](./SymbolInfoDouble.md)** — one `double` property (point, tick value, volume limits, swaps…). Uses `SymbolInfoDoubleProperty`.
* **[SymbolInfoInteger](./SymbolInfoInteger.md)** — one `int64` property (digits, spread, trade mode, session times…). Uses `SymbolInfoIntegerProperty`.
* **[SymbolInfoString](./SymbolInfoString.md)** — one `string` property (description, currencies, path…). Uses `SymbolInfoStringProperty`.

---

## 🧭 Plain English

* Need the **price now** → `symbolInfoTick()`.
* Need **one spec value** (point, digits, volume min…) → `symbolInfo*` with the right property enum.
* Need to **walk the catalog** → `symbolsTotal()` + `symbolName()`, or just `symbolList()`.
* A symbol **isn't quoting** → `symbolSelect()` it into Market Watch, then `symbolIsSynchronized()`.

> **🧱 Enum note:** the three `symbolInfo*` methods take a **property enum as input** (`set_type(...)`) and return a single value in `value()`.

---

## Quick choose

| If you need…                                  | Use                    | Returns                     | Enum used                       |
| --------------------------------------------- | ---------------------- | --------------------------- | ------------------------------- |
| Current bid/ask/last                          | `symbolInfoTick`       | `MrpcMqlTick`               | —                               |
| One numeric spec (point, tick value, vol min) | `symbolInfoDouble`     | `double` (`value()`)        | **Input:** `SymbolInfoDoubleProperty` |
| One integer spec (digits, spread, trade mode) | `symbolInfoInteger`    | `int64` (`value()`/`value_utc()`) | **Input:** `SymbolInfoIntegerProperty` |
| One text spec (description, currencies)       | `symbolInfoString`     | `string` (`value()`)        | **Input:** `SymbolInfoStringProperty` |
| Count symbols                                 | `symbolsTotal`         | `int32` (`total()`)         | —                               |
| Name by index                                 | `symbolName`           | `string` (`name()`)         | —                               |
| All names at once                             | `symbolList`           | repeated `string`           | —                               |
| Add/remove in Market Watch                    | `symbolSelect`         | `bool` (`success()`)        | —                               |
| Exists? custom?                               | `symbolExist`          | `bool` (`exists()`, `is_custom()`) | —                        |
| Data ready?                                   | `symbolIsSynchronized` | `bool` (`synchronized()`)   | —                               |

---

## ❌ Cross-refs & gotchas

* **Select before you quote.** Quotes/ticks/DOM flow only for **selected** symbols — call `symbolSelect(true)` first for unusual symbols, then `symbolIsSynchronized()`.
* **`value()` not `requested_value()`.** Unlike the Account group, `symbolInfo*` data uses `value()` (and `value_utc()` for time-based integer properties).
* **`name` vs `symbol`.** `symbolExist` uses the `name` field; everything else uses `symbol`.
* **Point vs tick size.** Use `SYMBOL_POINT` for price formatting and `SYMBOL_TRADE_TICK_SIZE` / `SYMBOL_TRADE_TICK_VALUE` for money math — they can differ.
* **Batch alternative.** For many fields across many symbols, [SymbolParamsMany](../6.%20Additional_Methods/SymbolParamsMany.md) is far cheaper than many single calls.

---

## 🟢 Minimal snippets

```cpp
// Current price
pb::SymbolInfoTickRequest tr; tr.set_symbol("EURUSD");
pb::MrpcMqlTick t = account.symbolInfoTick(tr);
std::printf("Bid %.5f Ask %.5f\n", t.bid(), t.ask());
```

```cpp
// One double property — point size
pb::SymbolInfoDoubleRequest dr; dr.set_symbol("EURUSD"); dr.set_type(pb::SYMBOL_POINT);
std::printf("Point: %.5f\n", account.symbolInfoDouble(dr).value());
```

```cpp
// One integer property — digits
pb::SymbolInfoIntegerRequest ir; ir.set_symbol("EURUSD"); ir.set_type(pb::SYMBOL_DIGITS);
std::printf("Digits: %lld\n", (long long)account.symbolInfoInteger(ir).value());
```

```cpp
// List every symbol
pb::SymbolListData list = account.symbolList();
std::printf("%d symbols\n", list.symbols_size());
```

---

## 🧱 Enums used in Symbol Information

| Method              | Input enum                   | Example values                                                     |
| ------------------- | ---------------------------- | ------------------------------------------------------------------ |
| `symbolInfoDouble`  | `SymbolInfoDoubleProperty`   | `SYMBOL_BID`(0), `SYMBOL_ASK`(3), `SYMBOL_POINT`(13), `SYMBOL_TRADE_TICK_VALUE`(14), `SYMBOL_VOLUME_MIN`(22) … (60 total) |
| `symbolInfoInteger` | `SymbolInfoIntegerProperty`  | `SYMBOL_DIGITS`(17), `SYMBOL_SPREAD`(19), `SYMBOL_TRADE_MODE`(22), `SYMBOL_TIME`(15) … |
| `symbolInfoString`  | `SymbolInfoStringProperty`   | `SYMBOL_DESCRIPTION`(9), `SYMBOL_CURRENCY_BASE`(5), `SYMBOL_CURRENCY_PROFIT`(6), `SYMBOL_PATH`(14) … |

Values are namespace-scope (`pb::SYMBOL_POINT`), not `enum class`.

---

## See also

* **Account:** [AccountSummary](../1.%20Account_Information/AccountSummary.md) — account-level snapshot
* **Additional:** [SymbolParamsMany](../6.%20Additional_Methods/SymbolParamsMany.md) — many symbols, many fields, one call
* **Streaming:** [OnSymbolTick](../7.%20Streaming_Methods/OnSymbolTick.md) — push-based live quotes
* **Trading:** [OrderSend](../4.%20Trading_Operations/OrderSend.md) — place an order on a selected symbol
