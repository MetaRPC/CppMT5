#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace metarpc {

struct AccountInfo {
    int64_t login;
    std::string currency;
    double balance;
    double equity;
    double margin;
    double freeMargin;
    double marginLevel;
    int leverage;
    std::string name;
    std::string server;
};

struct Quote {
    std::string symbol;
    double bid;
    double ask;
    int64_t timestamp;
};

enum class TradeAction {
    Buy,
    Sell,
    BuyLimit,
    SellLimit,
    BuyStop,
    SellStop,
    BuyStopLimit,
    SellStopLimit
};

struct OrderRequest {
    std::string symbol;
    TradeAction action;
    double volume;
    double price{0.0};
    double stopLoss{0.0};
    double takeProfit{0.0};
    int slippage{10};
    std::string comment;
};

struct OrderResult {
    int64_t ticket;
    int retcode;
    int64_t deal;
    int64_t order;
    double volume;
    double price;
    std::string comment;
};

class MT5Client {
public:
    MT5Client(const std::string& host = "mt5.mrpc.pro", int port = 443, const std::string& apiKey = "");
    ~MT5Client();

    static std::string computeDeterministicId(int64_t login, const std::string& password);
    std::string getId(int64_t login, const std::string& password);
    void setApiKey(const std::string& apiKey) { m_apiKey = apiKey; }
    void setId(const std::string& id) { m_id = id; }
    const std::string& getId() const { return m_id; }
    const std::string& getApiKey() const { return m_apiKey; }

    bool connect(int64_t login, const std::string& password);
    void disconnect();
    bool isConnected() const;

    AccountInfo getAccountInfo();
    void subscribeQuotes(const std::vector<std::string>& symbols, std::function<void(const Quote&)> callback);
    OrderResult orderSend(const OrderRequest& request);
    bool orderModify(int64_t ticket, double stopLoss, double takeProfit);
    bool orderClose(int64_t ticket, double volume);

private:
    std::string m_host;
    int m_port;
    std::string m_apiKey;
    std::string m_id;
    bool m_connected{false};
};

} // namespace metarpc
