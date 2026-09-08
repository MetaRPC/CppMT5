# Quotes & Market Data

Subscribing to tick data:

```cpp
client.subscribeQuotes({"EURUSD", "GBPUSD"}, [](const metarpc::Quote& q) {
    std::cout << q.symbol << " " << q.bid << " / " << q.ask << std::endl;
});
```
