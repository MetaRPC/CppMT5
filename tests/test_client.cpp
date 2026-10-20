// Offline unit test: no network. Verifies the id derivation, uuid5, timeframe helpers and
// the "not connected" behaviour. The live path is covered by fleet/runners/cpp/smoke.
#include "metarpc/mt5.hpp"
#include <cassert>
#include <iostream>

static void testIds() {
    // Legacy SHA-256 id (unchanged from the original stub so old ids stay stable).
    assert(metarpc::MT5Client::computeDeterministicId(12345678, "demo_password") == "6ce74465-7aa9-3a79-2bd8-e40241b50c43");
    // RFC 4122 uuid5 — reference values produced by Python's uuid.uuid5.
    assert(metarpc::MT5Client::uuid5(metarpc::MT5Client::NAMESPACE_DNS, "example.com") == "cfbff0d1-9375-5685-968c-48ce8b15ae17");
    assert(metarpc::MT5Client::uuid5(metarpc::MT5Client::NAMESPACE_DNS, "fleet-audit-CppMT5") == "6b3f105f-92bd-56f1-a80a-4416e4dbe474");
    assert(metarpc::MT5Client::uuid5(metarpc::MT5Client::NAMESPACE_FLEET, "fleet-audit-CppMT5") == "adf8e791-7b5f-53bc-91d8-cdce71962796");
}

static void testTimeframes() {
    assert(metarpc::timeframeSeconds(metarpc::Timeframe::M1) == 60);
    assert(metarpc::timeframeSeconds(metarpc::Timeframe::M15) == 900);
    assert(metarpc::timeframeSeconds(metarpc::Timeframe::H1) == 3600);
    assert(metarpc::timeframeSeconds(metarpc::Timeframe::H4) == 4 * 3600);
    assert(metarpc::timeframeSeconds(metarpc::Timeframe::D1) == 86400);
    assert((int)metarpc::Timeframe::H1 == 16385);
    assert((int)metarpc::Timeframe::D1 == 16408);
    assert(std::string(metarpc::timeframeName(metarpc::Timeframe::M5)) == "M5");
}

static void testClientOffline() {
    metarpc::MT5Client client("mt5.mrpc.pro", 443, "mrpc_test_key");
    assert(!client.isConnected());
    assert(client.getApiKey() == "mrpc_test_key");
    std::string id = client.getId(1001, "demo_pass");
    assert(!id.empty());
    assert(client.getId() == id);
    client.setId("adf8e791-7b5f-53bc-91d8-cdce71962796");
    assert(client.getId() == "adf8e791-7b5f-53bc-91d8-cdce71962796");

    // reconnect() before connect() must fail fast without touching the network.
    bool threw = false;
    try { client.reconnect(); } catch (const metarpc::ApiError& e) { threw = (e.code == "NOT_CONFIGURED"); }
    assert(threw);

    metarpc::ApiError bad("CONNECT_EX_INVALID_LOGIN_ERROR", "x");
    assert(bad.isInvalidLogin());
    metarpc::ApiError nf("TERMINAL_INSTANCE_NOT_FOUND", "x");
    assert(nf.isTerminalNotFound() && !nf.isInvalidLogin());

    metarpc::OrderResult r; r.retcode = 10009;
    assert(r.ok());
    r.retcode = 10013;
    assert(!r.ok());

    client.close();
    assert(!client.isConnected());
}

int main() {
    testIds();
    testTimeframes();
    testClientOffline();
    std::cout << "All CppMT5 offline tests passed." << std::endl;
    return 0;
}
