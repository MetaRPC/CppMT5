# ✅ Counting Open Positions

> **Request:** the number of currently open positions (`PositionsTotalData`).

**API Information:**

* **Low-level API:** `MT5Account::positionsTotal(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.TradeFunctions`
* **Proto definition:** `PositionsTotal` (defined in `mt5-term-api-trade-functions.proto`)

### RPC

* **Service:** `mt5_term_api.TradeFunctions`
* **Method:** `PositionsTotal(PositionsTotalRequest) → PositionsTotalReply`

---

## 💬 Just the essentials

* **What it is.** Returns a single count of open positions.
* **Why you need it.** A cheap "do I have anything open?" check before looping over details.
* **Sanity check.** `0` on a flat account; grows as you open trades.

> **Note.** This method takes **no arguments** — `MT5Account::positionsTotal()`.

---

## 🧱 Signature

```cpp
// Returns the open-position count. Throws ApiError / TransportError on failure.
pb::PositionsTotalData positionsTotal();
```

**Reply:**

```protobuf
PositionsTotalReply {
  oneof response {
    PositionsTotalData data  = 1;   // { int32 total_positions = 1; }
    Error              error = 2;
  }
}
```

---

## ⬆️ Output — `PositionsTotalData`

| Field            | C++ accessor        | C++ type  | Description                     |
| ---------------- | ------------------- | --------- | ------------------------------- |
| `TotalPositions` | `total_positions()` | `int32_t` | Number of open positions.       |

---

## 🧩 Notes & Tips

* **Positions ≠ pending orders.** This counts *open positions* only; for pending orders use [OpenedOrders](./OpenedOrders.md).
* **Guard before detail calls.** If this is `0`, skip the heavier [OpenedOrders](./OpenedOrders.md) call.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Simple check

```cpp
int32_t n = account.positionsTotal().total_positions();
std::printf("Open positions: %d\n", n);
```

### 2) Only fetch details when there's something to fetch

```cpp
if (account.positionsTotal().total_positions() > 0) {
    pb::OpenedOrdersRequest r;
    pb::OpenedOrdersData d = account.openedOrders(r);
    for (const auto& p : d.position_infos())
        std::printf("%s %.2f lots, P/L %.2f\n", p.symbol().c_str(), p.volume(), p.profit());
}
```

---

## 📚 See Also

* [OpenedOrders](./OpenedOrders.md) — full details of positions and pending orders
* [OpenedOrdersTickets](./OpenedOrdersTickets.md) — just the ticket numbers (lightweight)
* **Friendly layer:** `MT5Service::countOpenPositions()` / `MT5Sugar::countOpenPositions()`.
