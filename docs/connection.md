# Connection & Authentication

Managing connections with `metarpc::MT5Client`.

```cpp
#include <metarpc/mt5.hpp>

metarpc::MT5Client client("demo.broker.com", 443);

if (!client.connect(2005432, "password")) {
    std::cerr << "Failed to connect to MT5 server!" << std::endl;
    return -1;
}

if (client.isConnected()) {
    std::cout << "Ready for trading operations." << std::endl;
}

client.disconnect();
```
