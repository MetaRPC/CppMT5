// Offline transport test: an in-process gRPC server on 127.0.0.1 (random port) fakes the
// gateway so we can count how often each RPC is actually sent. Verifies that
//   - a trade call that fails at the transport level (UNAVAILABLE / UNKNOWN / INTERNAL /
//     DEADLINE_EXCEEDED) is sent exactly ONCE and surfaces as TradeOutcomeUnknownError
//     (which is still a TransportError);
//   - a trade answered in-band with TERMINAL_INSTANCE_NOT_FOUND re-attaches (ConnectEx) and is
//     re-sent once;
//   - a read that fails once with UNAVAILABLE is retried and succeeds;
//   - API_KEY_INVALID (in-band or in a status message) is not retried and keeps its code.
#include "metarpc/mt5_account.hpp"

#include <grpcpp/grpcpp.h>

#include "mt5-term-api-account-helper.grpc.pb.h"
#include "mt5-term-api-connection.grpc.pb.h"
#include "mt5-term-api-trading-helper.grpc.pb.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// Not "pb": protobuf >= 30 declares its own global ::pb namespace.
namespace api = mt5_term_api;
using metarpc::ApiError;
using metarpc::MT5Account;
using metarpc::TradeOutcomeUnknownError;
using metarpc::TransportError;

static int g_failures = 0;
#define CHECK(cond)                                                                     \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; \
            ++g_failures;                                                               \
        }                                                                               \
    } while (0)

// What the fake gateway answers. Reset before every scenario.
enum class Mode { Ok, Unavailable, Unknown, Internal, Deadline, NotFoundOnce, ApiKeyInvalid, ApiKeyInStatus, ReadUnavailableOnce };
static std::atomic<Mode> g_mode{Mode::Ok};
static std::atomic<int> g_connectEx{0}, g_orderSend{0}, g_orderModify{0}, g_orderClose{0}, g_summary{0};

static void reset(Mode m) {
    g_mode = m;
    g_connectEx = 0; g_orderSend = 0; g_orderModify = 0; g_orderClose = 0; g_summary = 0;
}

// Shared fake behaviour for trade RPCs: `n` is the post-increment call count.
template <class Reply>
static grpc::Status tradeAnswer(int n, Reply* reply) {
    switch (g_mode.load()) {
        case Mode::Unavailable:  return grpc::Status(grpc::StatusCode::UNAVAILABLE, "connection reset");
        case Mode::Unknown:      return grpc::Status(grpc::StatusCode::UNKNOWN, "stream removed");
        case Mode::Internal:     return grpc::Status(grpc::StatusCode::INTERNAL, "RST_STREAM");
        case Mode::Deadline:
            std::this_thread::sleep_for(std::chrono::milliseconds(400));   // client deadline is 100 ms
            return grpc::Status::OK;
        case Mode::NotFoundOnce:
            if (n == 1) { reply->mutable_error()->set_error_code("TERMINAL_INSTANCE_NOT_FOUND"); return grpc::Status::OK; }
            break;
        case Mode::ApiKeyInvalid:
            reply->mutable_error()->set_error_code("API_KEY_INVALID");
            reply->mutable_error()->set_error_message("unknown api key");
            return grpc::Status::OK;
        default: break;
    }
    reply->mutable_data();
    return grpc::Status::OK;
}

class FakeConnection final : public api::Connection::Service {
    grpc::Status ConnectEx(grpc::ServerContext*, const api::ConnectExRequest*, api::ConnectExReply* reply) override {
        ++g_connectEx;
        reply->mutable_data()->set_terminal_instance_guid("11111111-2222-3333-4444-555555555555");
        return grpc::Status::OK;
    }
};

class FakeAccountHelper final : public api::AccountHelper::Service {
    grpc::Status AccountSummary(grpc::ServerContext*, const api::AccountSummaryRequest*, api::AccountSummaryReply* reply) override {
        int n = ++g_summary;
        switch (g_mode.load()) {
            case Mode::ReadUnavailableOnce:
                if (n == 1) return grpc::Status(grpc::StatusCode::UNAVAILABLE, "connection reset");
                break;
            case Mode::ApiKeyInvalid:
                reply->mutable_error()->set_error_code("API_KEY_INVALID");
                reply->mutable_error()->set_error_message("unknown api key");
                return grpc::Status::OK;
            case Mode::ApiKeyInStatus:
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "API_KEY_INVALID: unknown api key");
            default: break;
        }
        reply->mutable_data();
        return grpc::Status::OK;
    }
};

class FakeTradingHelper final : public api::TradingHelper::Service {
    grpc::Status OrderSend(grpc::ServerContext*, const api::OrderSendRequest*, api::OrderSendReply* reply) override {
        return tradeAnswer(++g_orderSend, reply);
    }
    grpc::Status OrderModify(grpc::ServerContext*, const api::OrderModifyRequest*, api::OrderModifyReply* reply) override {
        return tradeAnswer(++g_orderModify, reply);
    }
    grpc::Status OrderClose(grpc::ServerContext*, const api::OrderCloseRequest*, api::OrderCloseReply* reply) override {
        return tradeAnswer(++g_orderClose, reply);
    }
};

// Run `fn` and expect a TradeOutcomeUnknownError for `op` carrying `grpcCode`.
template <class Fn>
static void expectOutcomeUnknown(const char* op, int grpcCode, Fn&& fn) {
    bool caught = false;
    try {
        fn();
    } catch (const TradeOutcomeUnknownError& e) {
        caught = true;
        CHECK(e.code() == "TRADE_OUTCOME_UNKNOWN");
        CHECK(e.operation() == op);
        CHECK(e.grpcCode == grpcCode);
        CHECK(std::string(e.what()).find("TRADE_OUTCOME_UNKNOWN") == 0);
        CHECK(std::string(e.what()).find("NOT re-sent") != std::string::npos);
        // Still a TransportError for existing handlers.
        const TransportError& base = e;
        CHECK(base.grpcCode == grpcCode);
    } catch (const std::exception& e) {
        std::cerr << op << ": unexpected exception: " << e.what() << "\n";
    }
    CHECK(caught);
}

int main() {
    FakeConnection connection;
    FakeAccountHelper helper;
    FakeTradingHelper trading;
    int port = 0;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&connection);
    builder.RegisterService(&helper);
    builder.RegisterService(&trading);
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server || port == 0) { std::cerr << "could not start the fake gateway\n"; return 2; }

    MT5Account acc("127.0.0.1", port, "test-key");
    acc.options().maxBackoffMs = 50;     // keep the retry back-off short
    reset(Mode::Ok);
    acc.connect(1001, "pw", "Fake-Server");
    CHECK(g_connectEx == 1);

    // 1) Trade + UNAVAILABLE -> exactly one OrderSend, typed error, no reconnect.
    reset(Mode::Unavailable);
    expectOutcomeUnknown("OrderSend", 14, [&] { acc.orderSend(api::OrderSendRequest{}); });
    CHECK(g_orderSend == 1);
    CHECK(g_connectEx == 0);
    // ... and it is still catchable as the pre-existing TransportError.
    reset(Mode::Unavailable);
    bool caughtAsTransport = false;
    try { acc.orderSend(api::OrderSendRequest{}); } catch (const TransportError& e) { caughtAsTransport = (e.grpcCode == 14); }
    CHECK(caughtAsTransport);
    CHECK(g_orderSend == 1);

    // 2) OrderModify + UNKNOWN, OrderClose + INTERNAL -> one call each.
    reset(Mode::Unknown);
    expectOutcomeUnknown("OrderModify", 2, [&] { acc.orderModify(api::OrderModifyRequest{}); });
    CHECK(g_orderModify == 1);
    reset(Mode::Internal);
    expectOutcomeUnknown("OrderClose", 13, [&] { acc.orderClose(api::OrderCloseRequest{}); });
    CHECK(g_orderClose == 1);

    // 3) OrderSend + DEADLINE_EXCEEDED (server slower than the trade deadline) -> one call.
    reset(Mode::Deadline);
    int savedTradeTimeout = acc.options().tradeTimeoutMs;
    acc.options().tradeTimeoutMs = 100;
    expectOutcomeUnknown("OrderSend", 4, [&] { acc.orderSend(api::OrderSendRequest{}); });
    acc.options().tradeTimeoutMs = savedTradeTimeout;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // let the slow handler finish
    CHECK(g_orderSend == 1);

    // 4) Trade answered in-band TERMINAL_INSTANCE_NOT_FOUND once -> ConnectEx + re-send (2 calls).
    reset(Mode::NotFoundOnce);
    try { acc.orderSend(api::OrderSendRequest{}); } catch (const std::exception& e) { std::cerr << "4: " << e.what() << "\n"; CHECK(false); }
    CHECK(g_orderSend == 2);
    CHECK(g_connectEx == 1);

    // 5) Read + UNAVAILABLE once -> retried and succeeds (2 calls).
    reset(Mode::ReadUnavailableOnce);
    try {
        acc.accountSummary();
    } catch (const std::exception& e) { std::cerr << "5: " << e.what() << "\n"; CHECK(false); }
    CHECK(g_summary == 2);

    // 6) API_KEY_INVALID in-band (read and trade) -> one call, no reconnect, code kept.
    reset(Mode::ApiKeyInvalid);
    bool apiKeyRead = false;
    try { acc.accountSummary(); } catch (const ApiError& e) { apiKeyRead = (e.code == "API_KEY_INVALID" && e.isApiKeyError()); }
    CHECK(apiKeyRead);
    CHECK(g_summary == 1);
    CHECK(g_connectEx == 0);
    reset(Mode::ApiKeyInvalid);
    bool apiKeyTrade = false;
    try { acc.orderSend(api::OrderSendRequest{}); } catch (const ApiError& e) { apiKeyTrade = (e.code == "API_KEY_INVALID" && e.isApiKeyError()); }
    CHECK(apiKeyTrade);
    CHECK(g_orderSend == 1);
    CHECK(g_connectEx == 0);

    // 7) API_KEY_INVALID in a transport status message -> not retried, message keeps the code.
    reset(Mode::ApiKeyInStatus);
    bool apiKeyStatus = false;
    try { acc.accountSummary(); } catch (const TransportError& e) {
        apiKeyStatus = std::string(e.what()).find("API_KEY_INVALID") != std::string::npos;
    }
    CHECK(apiKeyStatus);
    CHECK(g_summary == 1);
    CHECK(g_connectEx == 0);

    CHECK(ApiError("API_KEY_REQUIRED", "x").isApiKeyError());
    CHECK(!ApiError("TERMINAL_INSTANCE_NOT_FOUND", "x").isApiKeyError());

    acc.close();
    server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(2));
    if (g_failures) { std::cerr << g_failures << " check(s) failed\n"; return 1; }
    std::cout << "test_no_trade_replay: all checks passed\n";
    return 0;
}
