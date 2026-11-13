# Your First Project

A standalone C++ program that connects, reads the account, quotes a symbol, and places one tiny
order — then closes it. ~10 minutes.

## 0. Get an API key

To reach MetaRPC (`mt5.mrpc.pro:443`):

1. Register free at [https://mrpc.pro/signup](https://mrpc.pro/signup).
2. Generate a token in the portal at [https://mrpc.pro/my](https://mrpc.pro/my), under **API Keys**.
3. Expose it to your program: `export MRPC_API_KEY=your_token` (the SDK reads `MRPC_API_KEY`, falling back to `"TRIAL"`), or pass it to the `MT5Sugar` constructor.

You also need an MT5 **demo account** (login, password, server name) — see [MT5 for Beginners](MT5_For_Beginners.md). Keep the trade block below on a demo account you're happy to trade.

## 1. Project files

```
my_bot/
├── CMakeLists.txt
└── main.cpp
```

`CMakeLists.txt` — link against the CppMT5 library (built per [Getting Started](GETTING_STARTED.md),
installed or added via `add_subdirectory`):

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_bot CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(path/to/CppMT5 cppmt5)   # or find_package if installed
add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE metarpc_cppmt5)
```

## 2. Connect and read

```cpp
#include <iostream>
#include <metarpc/mt5_sugar.hpp>
using namespace metarpc;

int main() {
    MT5Sugar sugar("mt5.mrpc.pro", 443 /*, "YOUR_API_KEY" */);
    try {
        sugar.connect(5057947437, "YourPassword", "MetaQuotes-Demo");

        std::cout << "connected: " << sugar.isConnected()
                  << "  balance: " << sugar.getBalance() << "\n";

        auto px = sugar.getPriceInfo("EURUSD");
        std::cout << "EURUSD  bid " << px.bid << "  ask " << px.ask
                  << "  spread " << px.spread << "\n";
```

## 3. Size the trade and pre-check

```cpp
        double lots = sugar.calculatePositionSize("EURUSD", /*risk%*/ 1.0, /*stopPips*/ 50);
        std::string why;
        if (!sugar.canOpenPosition("EURUSD", lots, why)) {
            std::cout << "cannot open: " << why << "\n";
            return 0;
        }
```

## 4. Open, manage, close

```cpp
        uint64_t ticket = sugar.buyMarketWithPips("EURUSD", lots, /*sl*/ 50, /*tp*/ 100);
        if (!ticket) { std::cout << "order rejected\n"; return 0; }
        std::cout << "opened #" << ticket << " (" << lots << " lots)\n";

        // ... your logic would monitor it here ...

        std::cout << "close -> " << (sugar.closePosition((int64_t)ticket) ? "ok" : "failed") << "\n";

    } catch (const ApiError& e) {
        std::cerr << "API error: " << e.code << ": " << e.message << "\n";
        return 1;
    } catch (const TransportError& e) {
        std::cerr << "transport error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
```

## 5. Build and run

```bash
cmake -G Ninja -B build && cmake --build build
./build/my_bot
```

## Where to go next

- Keep it simple with [`MT5Sugar`](../API_Reference/MT5Sugar.md), or step down to
  [`MT5Service`](../API_Reference/MT5Service.md) for friendly structs and the full method set.
- Stream live prices: `sugar.service().subscribeTicks({"EURUSD"}, cb)` — see
  [gRPC Streaming](GRPC_STREAM_MANAGEMENT.md).
- Build a strategy loop: see the [Strategies](../Strategies/Strategies.Master.Overview.md).
- Browse `examples/demos/` for 14 runnable examples.
