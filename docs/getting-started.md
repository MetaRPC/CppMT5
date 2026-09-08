# Getting Started with CppMT5

## Prerequisites
- **C++17** compatible compiler
- **CMake 3.15+**

## Minimal Example

```cpp
#include <metarpc/mt5.hpp>
#include <iostream>

int main() {
    metarpc::MT5Client client("mt5.broker.com", 443);

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
