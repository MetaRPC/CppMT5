# ✅ Getting a Single String Symbol Property

> **Request:** one text symbol property (`SymbolInfoStringData`) from **MT5** — description, base/profit/margin currency, ISIN, path — selected by a property enum.

**API Information:**

* **Low-level API:** `MT5Account::symbolInfoString(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.MarketInfo`
* **Proto definition:** `SymbolInfoString` (defined in `mt5-term-api-market-info.proto`)

### RPC

* **Service:** `mt5_term_api.MarketInfo`
* **Method:** `SymbolInfoString(SymbolInfoStringRequest) → SymbolInfoStringReply`

---

## 💬 Just the essentials

* **What it is.** One RPC that returns a **single `string`** property of a symbol, chosen by a `SymbolInfoStringProperty` enum.
* **Why you need it.** Human-readable description, the symbol's currencies (for conversion), and its Market Watch path.
* **Sanity check.** `SYMBOL_CURRENCY_PROFIT` for EURUSD is `"USD"`.

---

## 🎯 Purpose

* Show a readable `SYMBOL_DESCRIPTION` in a UI.
* Read `SYMBOL_CURRENCY_BASE / _PROFIT / _MARGIN` to know how P/L and margin are denominated.
* Read `SYMBOL_PATH` to group symbols by folder.

---

## 🧱 Signature

```cpp
// Returns the single string property selected by req.type() for req.symbol().
// Throws ApiError / TransportError on failure.
pb::SymbolInfoStringData symbolInfoString(const pb::SymbolInfoStringRequest& req);
```

**Request / Reply:**

```protobuf
SymbolInfoStringRequest {
  string                   symbol = 1;
  SymbolInfoStringProperty type   = 2;
}

SymbolInfoStringReply {
  oneof response {
    SymbolInfoStringData data  = 1;   // { string value = 1; }
    Error                error = 2;
  }
}
```

---

## 🔽 Input

| Field    | C++ setter / type                                   | Description                   |
| -------- | --------------------------------------------------- | ----------------------------- |
| `symbol` | `set_symbol("EURUSD")` — string                     | The symbol.                   |
| `type`   | `set_type(pb::SYMBOL_DESCRIPTION)` — `SymbolInfoStringProperty` | Which string property to read. |

---

## ⬆️ Output — `SymbolInfoStringData`

| Field   | C++ accessor | C++ type             | Description                     |
| ------- | ------------ | -------------------- | ------------------------------- |
| `Value` | `value()`    | `const std::string&` | The value of the chosen property. |

---

## 🧱 Property enum — `SymbolInfoStringProperty`

| Constant                 | Value | Meaning                                   |
| ------------------------ | ----- | ----------------------------------------- |
| `SYMBOL_BASIS`           | 0     | Underlying asset of a derivative.         |
| `SYMBOL_CATEGORY`        | 1     | Symbol category.                          |
| `SYMBOL_COUNTRY`         | 2     | Country of the issuer.                    |
| `SYMBOL_SECTOR_NAME`     | 3     | Economic sector name.                     |
| `SYMBOL_INDUSTRY_NAME`   | 4     | Industry/branch name.                     |
| `SYMBOL_CURRENCY_BASE`   | 5     | Base currency.                            |
| `SYMBOL_CURRENCY_PROFIT` | 6     | Profit currency (P/L is denominated in it).|
| `SYMBOL_CURRENCY_MARGIN` | 7     | Margin currency.                          |
| `SYMBOL_BANK`            | 8     | Quote source / bank.                      |
| `SYMBOL_DESCRIPTION`     | 9     | Human-readable description.               |
| `SYMBOL_EXCHANGE`        | 10    | Exchange the symbol is traded on.         |
| `SYMBOL_FORMULA`         | 11    | Formula for a custom/synthetic symbol.    |
| `SYMBOL_ISIN`            | 12    | ISIN code.                                |
| `SYMBOL_PAGE`            | 13    | Web page with symbol info.                |
| `SYMBOL_PATH`            | 14    | Path in the Market Watch tree.            |

---

## 🧩 Notes & Tips

* **Currencies matter.** `SYMBOL_CURRENCY_PROFIT` tells you the currency of your floating P/L; if it differs from the account currency, a conversion is applied by the server.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Description + currencies

```cpp
std::string ss(MT5Account& a, const std::string& s, pb::SymbolInfoStringProperty p) {
    pb::SymbolInfoStringRequest r; r.set_symbol(s); r.set_type(p);
    return a.symbolInfoString(r).value();
}

std::printf("%s — base %s / profit %s / margin %s\n",
            ss(account, "EURUSD", pb::SYMBOL_DESCRIPTION).c_str(),
            ss(account, "EURUSD", pb::SYMBOL_CURRENCY_BASE).c_str(),
            ss(account, "EURUSD", pb::SYMBOL_CURRENCY_PROFIT).c_str(),
            ss(account, "EURUSD", pb::SYMBOL_CURRENCY_MARGIN).c_str());
```

### 2) Group symbols by path

```cpp
std::string path = ss(account, "EURUSD", pb::SYMBOL_PATH);   // e.g. "Forex\\Majors\\EURUSD"
std::printf("Market Watch path: %s\n", path.c_str());
```

---

## 📚 See Also

* [SymbolInfoDouble](./SymbolInfoDouble.md) — point, tick value, volume limits
* [SymbolInfoInteger](./SymbolInfoInteger.md) — digits, spread, trade mode
* [SymbolInfoTick](./SymbolInfoTick.md) — the full current tick in one call
* **Friendly layer:** `MT5Service::symbolParams("EURUSD")` returns `description`, `currencyBase`, `currencyProfit`, … already named.
