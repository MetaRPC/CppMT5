# API Reference

### `metarpc::MT5Client`
- `bool connect(int64_t login, const std::string& password)`
- `void disconnect()`
- `bool isConnected() const`
- `AccountInfo getAccountInfo()`
- `void subscribeQuotes(const std::vector<std::string>& symbols, std::function<void(const Quote&)> callback)`
- `OrderResult orderSend(const OrderRequest& request)`
- `bool orderModify(int64_t ticket, double stopLoss, double takeProfit)`
- `bool orderClose(int64_t ticket, double volume)`
