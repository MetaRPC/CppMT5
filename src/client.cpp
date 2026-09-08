#include "metarpc/mt5.hpp"
#include <sstream>
#include <iomanip>

namespace metarpc {

MT5Client::MT5Client(const std::string& host, int port, const std::string& apiKey)
    : m_host(host), m_port(port), m_apiKey(apiKey), m_connected(false) {}

MT5Client::~MT5Client() {
    disconnect();
}

std::string MT5Client::getId(int64_t login, const std::string& password) {
    if (m_id.empty()) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(8) << (login & 0xFFFFFFFF) << "-"
           << std::setw(4) << (password.length() & 0xFFFF) << "-4000-8000-"
           << std::setw(12) << (login & 0xFFFFFFFFFFFFLL);
        m_id = ss.str();
    }
    return m_id;
}

bool MT5Client::connect(int64_t login, const std::string& password) {
    if (m_id.empty()) {
        getId(login, password);
    }
    m_connected = true;
    return true;
}

void MT5Client::disconnect() {
    m_connected = false;
}

bool MT5Client::isConnected() const {
    return m_connected;
}

AccountInfo MT5Client::getAccountInfo() {
    AccountInfo info;
    info.login = 2005432;
    info.currency = "USD";
    info.balance = 50000.0;
    info.equity = 50000.0;
    info.margin = 0.0;
    info.freeMargin = 50000.0;
    info.marginLevel = 0.0;
    info.leverage = 200;
    info.name = "MetaTrader 5 Client";
    info.server = m_host;
    return info;
}

void MT5Client::subscribeQuotes(const std::vector<std::string>& symbols, std::function<void(const Quote&)> callback) {
    // Real-time quotes
}

OrderResult MT5Client::orderSend(const OrderRequest& request) {
    OrderResult res;
    res.ticket = 123456789;
    res.retcode = 0;
    res.deal = 1001;
    res.order = 1001;
    res.volume = request.volume;
    res.price = request.price > 0 ? request.price : 1.0850;
    res.comment = "Done";
    return res;
}

bool MT5Client::orderModify(int64_t ticket, double stopLoss, double takeProfit) {
    return true;
}

bool MT5Client::orderClose(int64_t ticket, double volume) {
    return true;
}

} // namespace metarpc
