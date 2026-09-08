# Getting Started with CppMT5

## Prerequisites
- **C++17** compatible compiler
- **CMake 3.15+**

## Minimal Example

```cpp
#include <metarpc/mt5.hpp>
#include <iostream>

int main() {
    metarpc::MT5Client client("mt5.mrpc.pro", 443);

    if (client.connect(2005432, "account_password")) {
        std::cout << "Connected to MT5!" << std::endl;

        metarpc::AccountInfo acc = client.getAccountInfo();
        std::cout << "Balance: " << acc.balance << " " << acc.currency << std::endl;

        client.subscribeQuotes({"EURUSD"}, [](const metarpc::Quote& quote) {
            std::cout << "[" << quote.symbol << "] Bid: " << quote.bid 
                      << " Ask: " << quote.ask << std::endl;
        });

        metarpc::OrderRequest req;
        req.symbol = "EURUSD";
        req.action = metarpc::TradeAction::Buy;
        req.volume = 0.01;
        req.comment = "CppMT5 Quickstart";

        metarpc::OrderResult result = client.orderSend(req);
        std::cout << "Order executed! Deal #" << result.deal << std::endl;
    }

    return 0;
}
```


> **Authentication Note**: Connecting to `mt5.mrpc.pro:443` requires a valid MetaRPC API key. Register for free at [https://mrpc.pro/signup](https://mrpc.pro/signup) and generate your token in [https://mrpc.pro/my](https://mrpc.pro/my).



---

## 🆔 Account ID Generation (`GetId`)

> ⚠️ **Important**: You must generate your deterministic account ID with `GetId` **firstly** before connecting or streaming.

MetaRPC endpoints require two essential credentials for all operations:
1. **`APIKey`**: Your personal authentication token from [https://mrpc.pro/my](https://mrpc.pro/my) (obtained by registering at [https://mrpc.pro/signup](https://mrpc.pro/signup)). Sent in the `APIKey` header.
2. **`id`**: A deterministic account GUID generated from your MetaTrader `user` (login number) and `password`. The same credentials always produce the exact same GUID.

### Calling GetId

#### Via REST API:
```bash
curl -X GET "https://mt5.mrpc.pro/GetId?user=YOUR_LOGIN&password=YOUR_PASSWORD" \
     -H "APIKey: YOUR_API_KEY"
```

Response:
```json
{
  "data": {
    "id": "e8d91060-c3d3-4f4d-8d2a-9e1b2c3d4e5f"
  }
}
```

#### Via gRPC:
Send a `GetIdRequest` with `user` and `password` to the connection service before calling `ConnectEx`.

Use this returned `id` GUID in the `id` header or session parameter for `ConnectEx`, `AccountSummary`, and all other terminal operations.
