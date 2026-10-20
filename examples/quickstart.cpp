// Quickstart: connect by broker server name, read the account, quote a symbol, place and
// close a tiny market order on a DEMO account.
//
//   export MRPC_API_KEY=mrpc_live_...
//   export MT5_LOGIN=12345678 MT5_PASSWORD=... MT5_SERVER=MetaQuotes-Demo
//   ./quickstart
#include "metarpc/mt5.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* login = std::getenv("MT5_LOGIN");
    const char* password = std::getenv("MT5_PASSWORD");
    const char* server = std::getenv("MT5_SERVER");
    if (!login || !password || !server) {
        std::cerr << "set MT5_LOGIN, MT5_PASSWORD, MT5_SERVER (and MRPC_API_KEY)" << std::endl;
        return 2;
    }

    metarpc::MT5Client client("mt5.mrpc.pro", 443 /*, apiKey: defaults to $MRPC_API_KEY */);
    // Stable terminal id: the gateway keeps ONE terminal per id. Never use a random id per run.
    client.setId(metarpc::MT5Client::uuid5(metarpc::MT5Client::NAMESPACE_FLEET, "quickstart-" + std::string(login)));

    try {
        client.connect(std::atoll(login), password, server, "cppmt5-quickstart");
        auto acc = client.accountSummary();
        std::cout << "Account " << acc.login << " (" << acc.name << ") " << acc.balance << " " << acc.currency
                  << " leverage 1:" << acc.leverage << " mode=" << (int)acc.tradeMode << std::endl;
        if (acc.tradeMode != metarpc::TradeMode::Demo) {
            std::cerr << "refusing to trade on a non-DEMO account" << std::endl;
            return 3;
        }

        const std::string symbol = "EURUSD";
        client.symbolSelect(symbol, true);
        auto info = client.symbolParams(symbol);
        auto tick = client.symbolInfoTick(symbol);
        std::cout << symbol << " bid=" << tick.bid << " ask=" << tick.ask << " digits=" << info.digits
                  << " volMin=" << info.volumeMin << std::endl;

        metarpc::OrderRequest req;
        req.symbol = symbol;
        req.action = metarpc::TradeAction::Buy;
        req.volume = info.volumeMin;
        req.magic = 123456;
        req.comment = "cppmt5-quickstart";
        auto res = client.orderSend(req);
        std::cout << "OrderSend retcode=" << res.retcode << " (" << res.retcodeName << ") order=" << res.order
                  << " deal=" << res.deal << " price=" << res.price << std::endl;
        if (res.ok()) {
            auto closeRes = client.orderClose(res.order);
            std::cout << "OrderClose retcode=" << closeRes.retcode << " (" << closeRes.retcodeName << ")" << std::endl;
        }
    } catch (const metarpc::ApiError& e) {
        std::cerr << "API error: " << e.what() << std::endl;
        return 1;
    } catch (const metarpc::TransportError& e) {
        std::cerr << "transport error: " << e.what() << std::endl;
        return 1;
    }
    // No client.disconnect(): the terminal stays alive on the gateway for the next run.
    return 0;
}
