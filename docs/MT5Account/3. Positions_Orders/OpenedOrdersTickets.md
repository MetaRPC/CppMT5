# ✅ Getting Open Ticket Numbers (Lightweight)

> **Request:** just the ticket numbers of open positions and pending orders (`OpenedOrdersTicketsData`) — no heavy detail payload.

**API Information:**

* **Low-level API:** `MT5Account::openedOrdersTickets(...)` (from `#include <metarpc/mt5_account.hpp>`, namespace `metarpc`)
* **gRPC service:** `mt5_term_api.AccountHelper`
* **Proto definition:** `OpenedOrdersTickets` (defined in `mt5-term-api-account-helper.proto`)

### RPC

* **Service:** `mt5_term_api.AccountHelper`
* **Method:** `OpenedOrdersTickets(OpenedOrdersTicketsRequest) → OpenedOrdersTicketsReply`

---

## 💬 Just the essentials

* **What it is.** Returns two lists of `int64` tickets: open **position** tickets and pending **order** tickets.
* **Why you need it.** A cheap way to detect *which* tickets exist (e.g. diffing against a previous snapshot) without the full [OpenedOrders](./OpenedOrders.md) payload.
* **Sanity check.** `opened_position_tickets_size()` matches [PositionsTotal](./PositionsTotal.md).

---

## 🧱 Signature

```cpp
// Returns ticket numbers of open positions and pending orders.
// Throws ApiError / TransportError on failure.
pb::OpenedOrdersTicketsData openedOrdersTickets(const pb::OpenedOrdersTicketsRequest& req);
```

**Request / Reply:**

```protobuf
OpenedOrdersTicketsRequest {}   // (empty)

OpenedOrdersTicketsReply {
  oneof response {
    OpenedOrdersTicketsData data  = 1;
    Error                   error = 2;
  }
}
```

---

## 🔽 Input

| Parameter | Type                                | Description                |
| --------- | ----------------------------------- | -------------------------- |
| `req`     | `const pb::OpenedOrdersTicketsRequest&` | Empty request message. |

---

## ⬆️ Output — `OpenedOrdersTicketsData`

| Field                   | C++ accessor                | Element type | Description                       |
| ----------------------- | --------------------------- | ------------ | --------------------------------- |
| `OpenedPositionTickets` | `opened_position_tickets()` | `int64`      | Tickets of open positions.        |
| `OpenedOrdersTickets`   | `opened_orders_tickets()`   | `int64`      | Tickets of pending orders.        |

Use `*_size()`, `(i)` indexing, or range-for.

---

## 🧩 Notes & Tips

* **Two lists.** Positions and pending orders are returned separately.
* **Great for change detection.** Compare consecutive snapshots to see what opened/closed without pulling full details.
* **Automatic reconnection / timeout / exceptions** as for every `MT5Account` method.

---

## 🔗 Usage Examples

### 1) Print all tickets

```cpp
pb::OpenedOrdersTicketsRequest r;
pb::OpenedOrdersTicketsData d = account.openedOrdersTickets(r);

std::printf("Positions: ");
for (int64_t t : d.opened_position_tickets()) std::printf("%lld ", (long long)t);
std::printf("\nPending: ");
for (int64_t t : d.opened_orders_tickets())   std::printf("%lld ", (long long)t);
std::printf("\n");
```

### 2) Detect newly opened positions

```cpp
#include <set>
std::set<int64_t> snapshot(MT5Account& a) {
    std::set<int64_t> s;
    pb::OpenedOrdersTicketsRequest r;
    for (int64_t t : a.openedOrdersTickets(r).opened_position_tickets()) s.insert(t);
    return s;
}

std::set<int64_t> before = snapshot(account);
// ... later ...
for (int64_t t : snapshot(account))
    if (!before.count(t)) std::printf("New position: #%lld\n", (long long)t);
```

---

## 📚 See Also

* [OpenedOrders](./OpenedOrders.md) — full details for each ticket
* [PositionsTotal](./PositionsTotal.md) — just the count
* **Friendly layer:** `MT5Service::openedTickets()` returns an `OpenedTickets` struct.
