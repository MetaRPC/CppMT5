# ✅ Getting Full Parameters for Many Symbols

> **Request:** detailed parameters for many symbols in one paginated call (`SymbolParamsManyData`) — every field per symbol, no per-property round-trips.

**API Information:**

* **Low-level API:** `MT5Account::symbolParamsMany(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `SymbolParamsMany` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `SymbolParamsMany(SymbolParamsManyRequest) → SymbolParamsManyReply`

---

## 💬 Just the essentials

* **What it is.** Returns a rich `SymbolParameters` record (100+ fields) for each matching symbol, paginated.
* **Why you need it.** The efficient way to snapshot specs for a whole watchlist — far cheaper than many `symbolInfo*` calls.
* **Sanity check.** `symbols_total()` ≥ `symbol_infos_size()`.

---

## 🧱 Signature

```cpp
// Returns detailed parameters for matching symbols (paginated).
// Throws ApiError / TransportError on failure.
pb::SymbolParamsManyData symbolParamsMany(const pb::SymbolParamsManyRequest& req);
```

**Request / Reply:**

```protobuf
SymbolParamsManyRequest {
  string                          symbol_name    = 1;   // filter (empty = all)
  AH_SYMBOL_PARAMS_MANY_SORT_TYPE sort_type      = 2;
  int32                           page_number    = 3;
  int32                           items_per_page = 4;
}

SymbolParamsManyReply {
  oneof response {
    SymbolParamsManyData data  = 1;
    Error                error = 2;
  }
}
```

---

## 🔽 Input

| Field            | C++ setter / type                        | Description                               |
| ---------------- | ---------------------------------------- | ----------------------------------------- |
| `symbol_name`    | `set_symbol_name("EUR")` — string        | Filter by name (empty = all symbols).     |
| `sort_type`      | `set_sort_type(...)` — sort enum         | Result ordering.                          |
| `page_number`    | `set_page_number(0)` — int32             | Zero-based page index.                    |
| `items_per_page` | `set_items_per_page(50)` — int32         | Page size.                                |

---

## ⬆️ Output — `SymbolParamsManyData`

| Field          | C++ accessor        | Type                     | Description                         |
| -------------- | ------------------- | ------------------------ | ----------------------------------- |
| `SymbolInfos`  | `symbol_infos()`    | repeated `SymbolParameters` | One rich record per symbol.      |
| `SymbolsTotal` | `symbols_total()`   | `int32`                  | Total matching symbols (all pages). |
| `PageNumber`   | `page_number()`     | `int32`                  | Current page.                       |
| `ItemsPerPage` | `items_per_page()`  | `int32`                  | Page size echoed back.              |

### `SymbolParameters` — a sample of the fields

Over 100 fields per symbol; commonly used ones include:

| Accessor                 | Type     | Description                     |
| ------------------------ | -------- | ------------------------------- |
| `name()`                 | `string` | Symbol name.                    |
| `digits()`               | `int`    | Price decimal digits.           |
| `point()`                | `double` | Point size.                     |
| `bid()` / `ask()`        | `double` | Current quotes.                 |
| `volume_min()` / `volume_max()` / `volume_step()` | `double` | Lot limits.     |
| `trade_tick_value()` / `trade_tick_size()` | `double` | Tick value/size.  |
| `currency_base()` / `currency_profit()` / `currency_margin()` | `string` | Currencies. |

> The full field set mirrors MT5's symbol specification; reach for the friendly `MT5Service::symbolParams()` struct when you want named C++ fields without protobuf.

---

## 🧩 Notes & Tips

* **Batch, don't loop.** One `symbolParamsMany` call replaces dozens of `symbolInfoDouble/Integer/String` calls per symbol.
* **Paginate.** Use `page_number` / `items_per_page`; read `symbols_total()` to know how many pages.
* **Large payloads.** Covered by `ClientOptions::maxReceiveMessageBytes` (64 MB default).
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Example

```cpp
pb::SymbolParamsManyRequest r;
r.set_symbol_name("");          // all symbols
r.set_page_number(0);
r.set_items_per_page(50);

pb::SymbolParamsManyData d = account.symbolParamsMany(r);
std::printf("Total symbols: %d (page has %d)\n", d.symbols_total(), d.symbol_infos_size());
for (const auto& s : d.symbol_infos())
    std::printf("%-10s digits=%d\n", s.name().c_str(), s.digits());
```

---

## 📚 See Also

* [SymbolInfoDouble](../2.%20Symbol_Information/SymbolInfoDouble.md) / [SymbolInfoInteger](../2.%20Symbol_Information/SymbolInfoInteger.md) / [SymbolInfoString](../2.%20Symbol_Information/SymbolInfoString.md) — single-property reads
* [TickValueWithSize](./TickValueWithSize.md) — just the tick value/size for P&L
* **Friendly layer:** `MT5Service::symbolParams("EURUSD")` returns a named `SymbolInfo`.
