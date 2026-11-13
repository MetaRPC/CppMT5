#pragma once
// MetaRPC CppMT5 — shared, protobuf-free building blocks used by all three layers.
//
// The error types and the transport options live here (not in mt5_account.hpp) so that
// strategy code built on MT5Service / MT5Sugar can `catch (const metarpc::ApiError&)` and
// tweak timeouts WITHOUT pulling in the 11 MB of generated protobuf headers. This header
// has no protobuf/gRPC dependency — only the standard library.

#include <stdexcept>
#include <string>

namespace metarpc {

#pragma region Errors
// ======================================================================================
// Errors (shared by all three layers)
// ======================================================================================

/// Thrown when the gateway or the MT5 terminal answers an RPC with an *error payload*
/// (as opposed to a transport failure). This is a "the server understood you and said no"
/// error: bad login, unknown symbol, trade rejected, terminal not found, etc.
///
/// `code` is the gateway's symbolic error code (e.g. "CONNECT_EX_INVALID_LOGIN_ERROR",
/// "TERMINAL_INSTANCE_NOT_FOUND"); the mql* fields carry the terminal's own
/// GetLastError()/trade return codes when the failure came from MT5 itself.
class ApiError : public std::runtime_error {
public:
    ApiError(const std::string& code, const std::string& message, int mqlErrorCode = 0,
             int mqlTradeCode = 0, const std::string& mqlTradeDescription = "",
             const std::string& where = "")
        : std::runtime_error((where.empty() ? "" : where + ": ") + code + ": " + message),
          code(code), message(message), mqlErrorCode(mqlErrorCode), mqlTradeCode(mqlTradeCode),
          mqlTradeDescription(mqlTradeDescription), where(where) {}
    std::string code;           ///< gateway error code, e.g. CONNECT_EX_INVALID_LOGIN_ERROR
    std::string message;        ///< human-readable message (includes mql hints when present)
    int mqlErrorCode;           ///< MQL5 GetLastError() style code (0 if none)
    int mqlTradeCode;           ///< MQL5 trade return code, e.g. 10009 = done (0 if none)
    std::string mqlTradeDescription;
    std::string where;          ///< the RPC that failed, e.g. "OrderSend"

    /// True for bad-credentials errors. The transport layer never retries these — a wrong
    /// password will not become right, so the call fails fast.
    bool isInvalidLogin() const {
        return code.find("INVALID_LOGIN") != std::string::npos || code.find("WRONG_PASSWORD") != std::string::npos;
    }
    /// True when the gateway no longer has the terminal instance. The transport layer treats
    /// these as recoverable and transparently re-issues ConnectEx before retrying the call.
    bool isTerminalNotFound() const {
        return code == "TERMINAL_INSTANCE_NOT_FOUND" || code == "TERMINAL_REGISTRY_TERMINAL_NOT_FOUND";
    }
};

/// Thrown when a gRPC call keeps failing at the *transport* level (the server could not be
/// reached or did not answer in time: UNAVAILABLE, DEADLINE_EXCEEDED, …) and the retry
/// budget in ClientOptions::maxRetries has been spent. `grpcCode` is the raw grpc::StatusCode.
class TransportError : public std::runtime_error {
public:
    TransportError(int grpcCode, const std::string& message)
        : std::runtime_error(message), grpcCode(grpcCode) {}
    int grpcCode;               ///< grpc::StatusCode as int (14 = UNAVAILABLE, 4 = DEADLINE_EXCEEDED)
};
#pragma endregion

#pragma region Options
// ======================================================================================
// Connection / transport options
// ======================================================================================

/// Per-client knobs for deadlines, the retry budget, TLS roots and logging. Obtain a
/// mutable reference with MT5Account::options() (or MT5Service::options()) and change
/// fields before the first call; they are read on every RPC. Timeouts are per-attempt
/// deadlines, not total budgets.
struct ClientOptions {
    int unaryTimeoutMs{30000};          ///< deadline for ordinary (read) RPCs
    int connectTimeoutMs{200000};       ///< deadline for ConnectEx (a cold terminal start can exceed 1 min)
    int tradeTimeoutMs{60000};          ///< deadline for OrderSend/Modify/Close
    int maxRetries{5};                  ///< transient-failure / reconnect retry budget per call
    int maxBackoffMs{5000};             ///< upper bound for the exponential back-off between retries
    int maxReceiveMessageBytes{64 * 1024 * 1024};  ///< gRPC max inbound message size (big histories)
    std::string rootCertsPath;          ///< PEM CA bundle; empty = autodetect common OS locations
    bool verbose{false};                ///< log retries/reconnects to stderr (or set MRPC_VERBOSE=1)
};
#pragma endregion

} // namespace metarpc
