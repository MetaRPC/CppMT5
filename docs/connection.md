# Connection & Authentication

Managing connections with `metarpc::MT5Client`.

```cpp
#include <metarpc/mt5.hpp>

metarpc::MT5Client client("mt5.mrpc.pro", 443);

if (!client.connect(2005432, "password")) {
    std::cerr << "Failed to connect to MT5 server!" << std::endl;
    return -1;
}

if (client.isConnected()) {
    std::cout << "Ready for trading operations." << std::endl;
}

client.disconnect();
```


> **Authentication Note**: Connecting to `mt5.mrpc.pro:443` requires a valid MetaRPC API key. Register for free at [https://mrpc.pro/signup](https://mrpc.pro/signup) and generate your token in [https://mrpc.pro/my](https://mrpc.pro/my).

