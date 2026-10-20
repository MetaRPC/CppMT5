# Your First Project in 10 Minutes (C++)

> **Hands-on Quick Start** - Create a working trading project with MetaTrader 5 and CppMT5 from scratch.

---

## Step 0: Obtain Your API Key

To connect to MetaRPC endpoints (`mt5.mrpc.pro:443`), obtain your API key:
1. Register for free at [https://mrpc.pro/signup](https://mrpc.pro/signup).
2. Generate your API token in your dashboard at [https://mrpc.pro/my](https://mrpc.pro/my).
3. Set your token in your environment or connection config.

---

## Step 1: Create Your Project

Create a new directory for your trading bot:

```bash
mkdir my_cppmt5_bot
cd my_cppmt5_bot
```

Get the SDK (it is not on a package registry):

```bash
git clone https://github.com/MetaRPC/CppMT5.git
```

---

## Step 2: Write Your Trading Code

Create your main application file and paste the following snippet:

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

---

## Step 3: Run the Program

Run your application:

```bash
cmake -B build -S . -DBUILD_EXAMPLES=ON
cmake --build build --config Release
./build/quickstart        # Windows: .\build\quickstart.exe or .\build\Release\quickstart.exe
```

Run it from the cloned repository: its `examples/quickstart` does the same thing.

---

## 🚀 Next Steps

Congratulations! You have successfully established a direct gRPC connection to MetaTrader 5. Next:
- Explore **[gRPC Streaming](GRPC_STREAM_MANAGEMENT.md)** to listen to live ticks.
- Check the **[API Reference](../API_Reference/MT5Account.md)** for all 40+ available terminal methods.
- Learn about high-level risk management and auto-normalization in **[MT5Sugar](../API_Reference/MT5Sugar.md)**.
