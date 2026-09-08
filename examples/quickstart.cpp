#include "metarpc/mt5.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    const char* envApiKey = std::getenv("MRPC_API_KEY");
    std::string apiKey = envApiKey ? envApiKey : "YOUR_API_KEY_HERE";

    metarpc::MT5Client client("mt5.mrpc.pro", 443, apiKey);

    int64_t login = 2005432;
    std::string password = "secret_password";

    std::cout << "Step 1: Generating Account ID (GetId)..." << std::endl;
    std::string accountId = client.getId(login, password);
    std::cout << "Account ID: " << accountId << std::endl;

    std::cout << "\nStep 2: Connecting to MetaTrader 5 (mt5.mrpc.pro:443)..." << std::endl;
    if (client.connect(login, password)) {
        std::cout << "Connected successfully to MT5!" << std::endl;

        std::cout << "\nStep 3: Querying Account Balance..." << std::endl;
        auto acc = client.getAccountInfo();
        std::cout << "Account: " << acc.login << " (" << acc.name << ")" << std::endl;
        std::cout << "Balance: " << acc.balance << " " << acc.currency << std::endl;

        std::cout << "\nStep 4: Executing Market Order..." << std::endl;
        metarpc::OrderRequest req;
        req.symbol = "EURUSD";
        req.action = metarpc::TradeAction::Buy;
        req.volume = 0.1;
        req.comment = "Cpp MT5 Bot";

        auto res = client.orderSend(req);
        std::cout << "Order placed! Deal: #" << res.deal << " Ticket: #" << res.ticket << std::endl;

        client.disconnect();
        std::cout << "\nDisconnected." << std::endl;
    }
    return 0;
}
