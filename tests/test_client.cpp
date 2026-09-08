#include "metarpc/mt5.hpp"
#include <cassert>
#include <iostream>

void testClientLifecycle() {
    metarpc::MT5Client client("mt5.mrpc.pro", 443, "mrpc_test_key");
    assert(!client.isConnected());
    assert(client.getApiKey() == "mrpc_test_key");

    std::string id = client.getId(1001, "demo_pass");
    assert(!id.empty());
    assert(client.getId() == id);

    bool connected = client.connect(1001, "demo_pass");
    assert(connected);
    assert(client.isConnected());

    metarpc::AccountInfo acc = client.getAccountInfo();
    assert(acc.login == 2005432);
    assert(acc.currency == "USD");
    assert(acc.balance == 50000.0);
    assert(acc.server == "mt5.mrpc.pro");

    metarpc::OrderRequest req;
    req.symbol = "EURUSD";
    req.action = metarpc::TradeAction::Buy;
    req.volume = 0.1;
    req.price = 1.0850;
    req.comment = "Test order";

    metarpc::OrderResult res = client.orderSend(req);
    assert(res.retcode == 0);
    assert(res.volume == 0.1);
    assert(res.price == 1.0850);
    assert(res.ticket > 0);

    assert(client.orderModify(res.ticket, 1.0800, 1.0900));
    assert(client.orderClose(res.ticket, 0.1));

    client.disconnect();
    assert(!client.isConnected());

    std::cout << "All CppMT5 client tests passed successfully!" << std::endl;
}

int main() {
    testClientLifecycle();
    return 0;
}
