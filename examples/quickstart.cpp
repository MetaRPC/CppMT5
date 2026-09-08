#include "metarpc/mt5.hpp"
#include <iostream>

int main() {
    metarpc::MT5Client client("mt5.broker.com", 443);

    if (client.connect(2005432, "secret")) {
        std::cout << "Connected to MT5!" << std::endl;

        auto acc = client.getAccountInfo();
        std::cout << "Account Balance: " << acc.balance << " " << acc.currency << std::endl;

        metarpc::OrderRequest req;
        req.symbol = "EURUSD";
        req.action = metarpc::TradeAction::Buy;
        req.volume = 0.1;
        req.comment = "C++ MT5 Strategy";

        auto res = client.orderSend(req);
        std::cout << "Order executed! Deal #" << res.deal << " Ticket #" << res.ticket << std::endl;
    }

    return 0;
}
