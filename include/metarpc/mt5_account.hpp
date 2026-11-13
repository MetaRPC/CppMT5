#pragma once
// MetaRPC CppMT5 — Layer 1 of 3: MT5Account (low-level).
//
// MT5Account is the thin, 1:1-with-the-wire layer. Every method maps to a single
// gRPC RPC, takes a protobuf *Request and returns the protobuf *Data payload of the
// reply. It owns the gRPC channel, the service stubs, the connection/identity state
// and the transport machinery: transient-failure retry with exponential back-off and
// an automatic ConnectEx re-attach on TERMINAL_INSTANCE_NOT_FOUND. It does NOT convert
// protobuf into friendly structs and does NOT do UTC<->server time conversion — that is
// MT5Service's job (Layer 2). MT5Sugar (Layer 3) sits on top of MT5Service.
//
// Because this layer speaks protobuf, including this header pulls in the generated
// headers (built from proto/mt5_term_api.desc). Strategy code that only needs friendly types should include
// <metarpc/mt5_service.hpp> or <metarpc/mt5_sugar.hpp> instead, which hide protobuf
// behind a pimpl.
//
// Threading: a single MT5Account owns one gRPC channel. Unary calls are synchronous and
// may be issued from multiple threads, but connect()/reconnect()/close() mutate the
// channel and should not race with in-flight calls. Every streaming subscription runs on
// its own background thread (see AccountStream).
//
// Errors are exceptions — no error codes are returned:
//   - ApiError       : the gateway/terminal answered with an error (code + message).
//   - TransportError : gRPC transport failure after the retry budget was spent.

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "metarpc/mt5_common.hpp"   // ApiError, TransportError, ClientOptions (protobuf-free)

#include "mt5-term-api-connection.pb.h"
#include "mt5-term-api-account-helper.pb.h"
#include "mt5-term-api-account-information.pb.h"
#include "mt5-term-api-market-info.pb.h"
#include "mt5-term-api-subscriptions.pb.h"
#include "mt5-term-api-trading-helper.pb.h"
#include "mt5-term-api-trade-functions.pb.h"
#include "mrpc-mt5-error.pb.h"

namespace metarpc {

namespace pb = mt5_term_api;

// ApiError, TransportError and ClientOptions are defined in mt5_common.hpp (protobuf-free).

#pragma region Streaming types
// ======================================================================================
// Low-level server-stream subscription — delivers raw protobuf payloads on a background
// thread. One generic handle backs every On* stream (ticks, trades, position profit, …);
// MT5Service wraps each into a friendly, typed stream.
// ======================================================================================

/// Handle to a running server-stream subscription (OnSymbolTick / OnTrade / OnPositionProfit /
/// OnTradeTransaction / OnPositionsAndPendingOrdersTickets).
///
/// The callback passed to the matching MT5Account::onX() method runs on a dedicated background
/// reader thread for as long as this handle is alive. Destroying the handle (or calling stop())
/// cancels the stream and joins that thread, so keep the handle in scope while you want data.
/// All accessors are safe to call from any thread.
class AccountStream {
public:
    ~AccountStream();
    /// Cancel the stream and join the reader thread. Idempotent; also called by the destructor.
    void stop();
    /// False once the stream has ended (cancelled, or the server closed it).
    bool running() const;
    /// Number of payloads delivered to the callback so far.
    uint64_t count() const;
    /// The error that ended the stream, or "" if it ended cleanly / was cancelled.
    std::string lastError() const;
    struct Impl;
private:
    friend class MT5Account;
    AccountStream();
    std::unique_ptr<Impl> m_impl;
};
#pragma endregion

// ======================================================================================
// MT5Account — Layer 1 (low-level, protobuf in / protobuf out)
// ======================================================================================
class MT5Account {
public:
    /// Construct a client bound to a gateway. Nothing connects yet — call connect().
    ///
    /// @param host    gateway host (default "mt5.mrpc.pro").
    /// @param port    gateway port (443 = TLS, which is the default).
    /// @param apiKey  gateway API key; if empty, falls back to the MRPC_API_KEY env var,
    ///                then to the literal "TRIAL".
    /// @param name    optional terminal instance display name.
    explicit MT5Account(const std::string& host = "mt5.mrpc.pro", int port = 443,
                        const std::string& apiKey = "", const std::string& name = "");
    ~MT5Account();
    MT5Account(const MT5Account&) = delete;
    MT5Account& operator=(const MT5Account&) = delete;

#pragma region Identity
    // --- identity -----------------------------------------------------------------------
    // The "terminal id" is the stable key the gateway uses to find (and re-attach to) your
    // running terminal across reconnects and process restarts. Set it once, keep it constant.

    static constexpr const char* NAMESPACE_DNS = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
    static constexpr const char* NAMESPACE_FLEET = "6f1c3b2e-7a4d-4c0e-9a1f-0000f1ee0001";

    /// Compute an RFC 4122 v5 (SHA-1) UUID from a namespace UUID and a name. This is the
    /// fleet convention for deriving a stable terminal id from something human (a strategy
    /// name, an account label). Deterministic and side-effect free.
    ///
    /// @param namespaceUuid  a UUID string, e.g. NAMESPACE_FLEET.
    /// @param name           any string; the same inputs always yield the same UUID.
    /// @returns              the v5 UUID as a lowercase "8-4-4-4-12" string.
    static std::string uuid5(const std::string& namespaceUuid, const std::string& name);

    /// Legacy deterministic id: a UUID derived from SHA-256 of "login:password", kept for
    /// compatibility with older code that keyed terminals by credentials. Prefer uuid5() for
    /// new code (it does not hash the password). Deterministic and side-effect free.
    static std::string computeDeterministicId(int64_t login, const std::string& password);

    /// Legacy convenience: compute the deterministic id for these credentials, store it as
    /// this client's terminal id, and return it. Equivalent to setId(computeDeterministicId()).
    std::string getId(int64_t login, const std::string& password);
    /// Set the terminal id explicitly. Call this BEFORE connect(). It must stay stable per
    /// account so the gateway re-attaches to the same terminal instead of starting a new one.
    void setId(const std::string& id);
    const std::string& getId() const;                ///< the terminal id currently in use

    void setApiKey(const std::string& apiKey);       ///< override the gateway API key at runtime
    const std::string& getApiKey() const;
    void setName(const std::string& name);           ///< set the terminal instance display name
    const std::string& getName() const;

    /// Mutable transport/timeout options (deadlines, retries, TLS roots, verbosity).
    ClientOptions& options();
#pragma endregion

#pragma region Connection
    // --- connection ---------------------------------------------------------------------

    /// Connect: start or re-attach the terminal via ConnectEx, by broker server name.
    ///
    /// Stores the credentials so the transport layer can auto-reconnect later, opens the
    /// gRPC channel, and blocks until the terminal is ready. A cold start can take more than
    /// a minute, which is why its deadline (ClientOptions::connectTimeoutMs) is large. If no
    /// terminal id was set, a deterministic one is derived from the credentials.
    ///
    /// @param login           trading account number.
    /// @param password        account password (master or investor).
    /// @param server          broker server / cluster name, e.g. "MetaQuotes-Demo".
    /// @param name            optional terminal instance name (overrides the constructor's).
    /// @param timeoutSeconds  how long the gateway waits for the terminal to come up.
    /// @throws ApiError       on bad credentials or a gateway refusal; TransportError on a
    ///                        transport failure that outlived the retry budget.
    void connect(int64_t login, const std::string& password, const std::string& server,
                 const std::string& name = "", int timeoutSeconds = 120);
    /// Re-issue ConnectEx with the stored credentials. Normally unnecessary — the transport
    /// layer reconnects automatically on TERMINAL_INSTANCE_NOT_FOUND — but call it after a
    /// manual close() to re-attach. @throws ApiError / TransportError.
    void reconnect();
    bool isConnected() const;                        ///< true after a successful connect/reconnect
    std::string terminalGuid() const;                ///< the id the gateway returned (== getId() normally)

    /// Send the Disconnect RPC: the gateway STOPS the terminal. Do NOT call this from a
    /// long-running strategy's normal shutdown unless you really want the terminal gone — use
    /// close() to keep it alive for the next run. @throws on transport failure.
    void disconnect(const std::string& reason = "");
    /// Drop the local gRPC channel only. The terminal keeps running on the gateway, so a later
    /// connect()/reconnect() re-attaches to the same instance. Safe to call repeatedly.
    void close();
#pragma endregion

#pragma region Time
    // --- time ---------------------------------------------------------------------------

    /// Broker server clock minus UTC, in seconds. The gateway speaks broker server time on the
    /// wire (tick/bar/deal times, history windows); this offset is learned from one
    /// AccountSummary and cached. Layer 2 uses it to present everything in UTC. A positive
    /// value means the broker clock is ahead of UTC.
    /// @throws ApiError / TransportError if an AccountSummary is needed and fails.
    int64_t serverOffsetSeconds();
#pragma endregion

#pragma region Account
    // --- account ------------------------------------------------------------------------

    /// Full account snapshot in a single round-trip: login, balance, equity, credit, leverage,
    /// trade mode, company, currency and the broker server time. This is the cheapest way to
    /// read account state and a good first call after connect(), since it also refreshes the
    /// cached server-time offset.
    ///
    /// @param req  AccountSummaryRequest — an empty message; the default is fine.
    /// @returns    AccountSummaryData with the fields above.
    /// @throws     ApiError / TransportError.
    pb::AccountSummaryData accountSummary(const pb::AccountSummaryRequest& req = {});

    /// One double-typed account property (e.g. balance, equity, margin, free margin, profit).
    /// Use this when you need a single number; prefer accountSummary() when you need several.
    /// @param req  AccountInfoDoubleRequest with property_id (AccountInfoDoublePropertyType).
    /// @returns    AccountInfoDoubleData whose requested_value holds the number.
    /// @throws     ApiError / TransportError.
    pb::AccountInfoDoubleData accountInfoDouble(const pb::AccountInfoDoubleRequest& req);

    /// One integer-typed account property (e.g. login, leverage, trade mode, margin mode).
    /// @param req  AccountInfoIntegerRequest with property_id (AccountInfoIntegerPropertyType).
    /// @returns    AccountInfoIntegerData whose requested_value holds the number.
    /// @throws     ApiError / TransportError.
    pb::AccountInfoIntegerData accountInfoInteger(const pb::AccountInfoIntegerRequest& req);

    /// One string-typed account property (e.g. name, server, currency, company).
    /// @param req  AccountInfoStringRequest with property_id (AccountInfoStringPropertyType).
    /// @returns    AccountInfoStringData whose requested_value holds the string.
    /// @throws     ApiError / TransportError.
    pb::AccountInfoStringData accountInfoString(const pb::AccountInfoStringRequest& req);
#pragma endregion

#pragma region Symbols / quotes
    // --- symbols / quotes ---------------------------------------------------------------

    /// Add or remove a symbol from Market Watch. A symbol usually must be selected before the
    /// terminal streams its quotes or exposes full params.
    /// @param req  SymbolSelectRequest with symbol and select (true = add, false = remove).
    /// @returns    SymbolSelectData whose success reports the outcome.
    /// @throws     ApiError / TransportError.
    pb::SymbolSelectData symbolSelect(const pb::SymbolSelectRequest& req);

    /// Whether the broker knows a symbol at all (standard or custom).
    /// @param req  SymbolExistRequest with name.
    /// @returns    SymbolExistData with exists / is_custom.
    /// @throws     ApiError / TransportError.
    pb::SymbolExistData symbolExist(const pb::SymbolExistRequest& req);

    /// Whether a symbol's rates and parameters are fully synchronised on the terminal. Freshly
    /// selected symbols may need a moment before this is true.
    /// @param req  SymbolIsSynchronizedRequest with symbol.
    /// @returns    SymbolIsSynchronizedData with synchronized.
    /// @throws     ApiError / TransportError.
    pb::SymbolIsSynchronizedData symbolIsSynchronized(const pb::SymbolIsSynchronizedRequest& req);

    /// Every symbol name known to the terminal (the full broker list, not just Market Watch).
    /// @param req  SymbolListRequest — empty; the default is fine.
    /// @returns    SymbolListData with the repeated symbols field.
    /// @throws     ApiError / TransportError.
    pb::SymbolListData symbolList(const pb::SymbolListRequest& req = {});

    /// Latest tick for a symbol: bid, ask, last, volume and flags. The time fields are raw
    /// broker server time — Layer 2 converts them to UTC.
    /// @param req  SymbolInfoTickRequest with symbol.
    /// @returns    MrpcMqlTick (note: this RPC returns the tick message directly, not an *Data).
    /// @throws     ApiError / TransportError.
    pb::MrpcMqlTick symbolInfoTick(const pb::SymbolInfoTickRequest& req);

    /// One double symbol property (e.g. point, tick size/value, volume min/max/step, swap).
    /// The property id is the PROTO enum SymbolInfoDoubleProperty, not the MQL5 constant.
    /// @param req  SymbolInfoDoubleRequest with symbol and type.
    /// @returns    SymbolInfoDoubleData whose value holds the number.
    /// @throws     ApiError / TransportError.
    pb::SymbolInfoDoubleData symbolInfoDouble(const pb::SymbolInfoDoubleRequest& req);

    /// One integer symbol property (e.g. digits, spread, stops level, trade mode). The property
    /// id is the PROTO enum SymbolInfoIntegerProperty.
    /// @param req  SymbolInfoIntegerRequest with symbol and type.
    /// @returns    SymbolInfoIntegerData whose value holds the number.
    /// @throws     ApiError / TransportError.
    pb::SymbolInfoIntegerData symbolInfoInteger(const pb::SymbolInfoIntegerRequest& req);

    /// One string symbol property (e.g. description, base/profit/margin currency, path). The
    /// property id is the PROTO enum SymbolInfoStringProperty.
    /// @param req  SymbolInfoStringRequest with symbol and type.
    /// @returns    SymbolInfoStringData whose value holds the string.
    /// @throws     ApiError / TransportError.
    pb::SymbolInfoStringData symbolInfoString(const pb::SymbolInfoStringRequest& req);

    /// Full parameter set for one or many symbols in one call: prices, digits/point, contract
    /// and tick sizing, volume limits, swap and trade modes, filling modes, etc. Much cheaper
    /// than many symbolInfo* calls when you need several fields.
    /// @param req  SymbolParamsManyRequest; set symbol_name to filter, or leave empty for all.
    /// @returns    SymbolParamsManyData with the repeated symbol_infos.
    /// @throws     ApiError / TransportError.
    pb::SymbolParamsManyData symbolParamsMany(const pb::SymbolParamsManyRequest& req);

    /// Count of symbols (all, or just the Market Watch selection, per req.mode).
    /// @param req  SymbolsTotalRequest with mode.
    /// @returns    SymbolsTotalData with total.
    /// @throws     ApiError / TransportError.
    pb::SymbolsTotalData symbolsTotal(const pb::SymbolsTotalRequest& req);

    /// Symbol name by position index — pair with symbolsTotal() to enumerate symbols.
    /// @param req  SymbolNameRequest with index and selected (Market Watch only?).
    /// @returns    SymbolNameData with name.
    /// @throws     ApiError / TransportError.
    pb::SymbolNameData symbolName(const pb::SymbolNameRequest& req);

    /// Initial/maintenance margin rate for a symbol and order type.
    /// @param req  SymbolInfoMarginRateRequest with symbol and order_type.
    /// @returns    SymbolInfoMarginRateData with the two rates.
    /// @throws     ApiError / TransportError.
    pb::SymbolInfoMarginRateData symbolInfoMarginRate(const pb::SymbolInfoMarginRateRequest& req);

    /// Quote-session start/end times for a symbol on a given weekday + session index.
    /// @param req  SymbolInfoSessionQuoteRequest with symbol, day_of_week and session_index.
    /// @returns    SymbolInfoSessionQuoteData with from/to (raw broker server time).
    /// @throws     ApiError / TransportError.
    pb::SymbolInfoSessionQuoteData symbolInfoSessionQuote(const pb::SymbolInfoSessionQuoteRequest& req);

    /// Trade-session start/end times for a symbol on a given weekday + session index.
    /// @param req  SymbolInfoSessionTradeRequest with symbol, day_of_week and session_index.
    /// @returns    SymbolInfoSessionTradeData with from/to (raw broker server time).
    /// @throws     ApiError / TransportError.
    pb::SymbolInfoSessionTradeData symbolInfoSessionTrade(const pb::SymbolInfoSessionTradeRequest& req);

    /// Tick value and contract size for one or more symbols in a single call (for P&L / sizing).
    /// @param req  TickValueWithSizeRequest with symbol_names.
    /// @returns    TickValueWithSizeData with one entry per symbol.
    /// @throws     ApiError / TransportError.
    pb::TickValueWithSizeData tickValueWithSize(const pb::TickValueWithSizeRequest& req);
#pragma endregion

#pragma region Market depth (DOM)
    // --- market depth (DOM) -------------------------------------------------------------

    /// Subscribe the terminal to a symbol's order book (Depth of Market). Must be called before
    /// marketBookGet() returns data for that symbol.
    /// @param req  MarketBookAddRequest with symbol.
    /// @returns    MarketBookAddData.
    /// @throws     ApiError / TransportError.
    pb::MarketBookAddData marketBookAdd(const pb::MarketBookAddRequest& req);

    /// Current order-book snapshot (bids/asks at each price level) for a subscribed symbol.
    /// @param req  MarketBookGetRequest with symbol.
    /// @returns    MarketBookGetData with the book entries.
    /// @throws     ApiError / TransportError.
    pb::MarketBookGetData marketBookGet(const pb::MarketBookGetRequest& req);

    /// Unsubscribe the terminal from a symbol's order book (release the DOM).
    /// @param req  MarketBookReleaseRequest with symbol.
    /// @returns    MarketBookReleaseData.
    /// @throws     ApiError / TransportError.
    pb::MarketBookReleaseData marketBookRelease(const pb::MarketBookReleaseRequest& req);
#pragma endregion

#pragma region Bars
    // --- bars ---------------------------------------------------------------------------

    /// OHLC bars for a symbol/timeframe over a time window. Both the window you pass and the
    /// bar times you get back are raw broker server time (Layer 2 does the UTC conversion). A
    /// zero from/to with a max-bars limit asks for the newest N bars.
    /// @param req  PriceHistoryRequest with symbol, timeframe, from/to and optional max_bars.
    /// @returns    PriceHistoryData with the repeated bars.
    /// @throws     ApiError / TransportError.
    pb::PriceHistoryData priceHistory(const pb::PriceHistoryRequest& req);
#pragma endregion

#pragma region Trading
    // --- trading ------------------------------------------------------------------------

    /// Place a market or pending order. Uses the trade deadline (ClientOptions::tradeTimeoutMs).
    /// Success is MT5 return code 10009 (done) — always inspect returned_code in the reply;
    /// a thrown ApiError means the request never reached the trade server as valid.
    /// @param req  OrderSendRequest (symbol, operation, volume, price, SL/TP, expiration, …).
    /// @returns    OrderSendData with returned_code, deal/order tickets, fill price, etc.
    /// @throws     ApiError / TransportError.
    pb::OrderSendData orderSend(const pb::OrderSendRequest& req);

    /// Modify an existing order or position: stop loss, take profit, and (for pending orders)
    /// price and stop-limit. As with orderSend, check returned_code in the reply.
    /// @param req  OrderModifyRequest with ticket and the new levels.
    /// @returns    OrderModifyData with the return code.
    /// @throws     ApiError / TransportError.
    pb::OrderModifyData orderModify(const pb::OrderModifyRequest& req);

    /// Close (fully or partially) a position, or delete a pending order, by ticket. The reply
    /// reports the return code and the close mode the gateway applied.
    /// @param req  OrderCloseRequest with ticket, volume (0 = whole position) and slippage.
    /// @returns    OrderCloseData with returned_code and close_mode.
    /// @throws     ApiError / TransportError.
    pb::OrderCloseData orderClose(const pb::OrderCloseRequest& req);

    /// Snapshot of everything currently live on the account: open positions and pending orders.
    /// @param req  OpenedOrdersRequest (sort mode).
    /// @returns    OpenedOrdersData with repeated position_infos and opened_orders.
    /// @throws     ApiError / TransportError.
    pb::OpenedOrdersData openedOrders(const pb::OpenedOrdersRequest& req);

    /// Number of currently open positions. Takes no request (a cheap count, no snapshot).
    /// @returns    PositionsTotalData with total_positions.
    /// @throws     ApiError / TransportError.
    pb::PositionsTotalData positionsTotal();

    /// Just the tickets of what is live: open position tickets and pending order tickets. Much
    /// lighter than openedOrders() when you only need the ids.
    /// @param req  OpenedOrdersTicketsRequest.
    /// @returns    OpenedOrdersTicketsData with the two ticket lists.
    /// @throws     ApiError / TransportError.
    pb::OpenedOrdersTicketsData openedOrdersTickets(const pb::OpenedOrdersTicketsRequest& req);

    /// Validate an order request without sending it (margin, free margin, retcode), like MQL5
    /// OrderCheck. Use before orderSend() to pre-flight a trade.
    /// @param req  OrderCheckRequest with the mql_trade_request to validate.
    /// @returns    OrderCheckData with the check result.
    /// @throws     ApiError / TransportError.
    pb::OrderCheckData orderCheck(const pb::OrderCheckRequest& req);

    /// Margin required to open a given order (symbol, type, volume, price), like MQL5 OrderCalcMargin.
    /// @param req  OrderCalcMarginRequest.
    /// @returns    OrderCalcMarginData with the margin.
    /// @throws     ApiError / TransportError.
    pb::OrderCalcMarginData orderCalcMargin(const pb::OrderCalcMarginRequest& req);

    /// Profit a given price move would yield for a position (symbol, type, volume, open/close
    /// price), like MQL5 OrderCalcProfit.
    /// @param req  OrderCalcProfitRequest.
    /// @returns    OrderCalcProfitData with the profit.
    /// @throws     ApiError / TransportError.
    pb::OrderCalcProfitData orderCalcProfit(const pb::OrderCalcProfitRequest& req);
#pragma endregion

#pragma region History
    // --- history ------------------------------------------------------------------------

    /// Closed orders and deals within a time window. The window on the wire is raw broker
    /// server time (Layer 2 accepts UTC and converts). Large windows can be big — the receive
    /// size limit and a doubled deadline account for that.
    /// @param req  OrderHistoryRequest with the from/to window, sort mode and paging.
    /// @returns    OrdersHistoryData with the repeated history_data (each an order and/or deal).
    /// @throws     ApiError / TransportError.
    pb::OrdersHistoryData orderHistory(const pb::OrderHistoryRequest& req);

    /// Closed positions within a time window (raw broker server time on the wire).
    /// @param req  PositionsHistoryRequest with the open-time window and sort type.
    /// @returns    PositionsHistoryData with the repeated history_positions.
    /// @throws     ApiError / TransportError.
    pb::PositionsHistoryData positionsHistory(const pb::PositionsHistoryRequest& req);
#pragma endregion

#pragma region Streaming
    // --- streaming ----------------------------------------------------------------------

    // Every onX() below opens a server stream and pumps raw protobuf payloads to `callback` on a
    // dedicated background thread. The returned AccountStream handle owns the subscription: keep
    // it alive to keep receiving, and destroy it (or call stop()) to cancel. The callback must
    // not block for long — it runs on the reader thread and holds up the next payload. Exceptions
    // thrown by the callback are swallowed. Payload times are raw broker server time.

    /// Subscribe to live ticks for one or more symbols.
    /// @param req  OnSymbolTickRequest with symbol_names.
    /// @param callback  invoked for every pb::OnSymbolTickData.
    /// @throws ApiError / TransportError while opening the stream.
    std::unique_ptr<AccountStream> onSymbolTick(
        const pb::OnSymbolTickRequest& req,
        std::function<void(const pb::OnSymbolTickData&)> callback);

    /// Subscribe to trade events (position/order changes). @param callback  pb::OnTradeData.
    std::unique_ptr<AccountStream> onTrade(
        const pb::OnTradeRequest& req,
        std::function<void(const pb::OnTradeData&)> callback);

    /// Subscribe to the detailed trade-transaction log. @param callback  pb::OnTradeTransactionData.
    std::unique_ptr<AccountStream> onTradeTransaction(
        const pb::OnTradeTransactionRequest& req,
        std::function<void(const pb::OnTradeTransactionData&)> callback);

    /// Subscribe to periodic position-profit updates. @param callback  pb::OnPositionProfitData.
    std::unique_ptr<AccountStream> onPositionProfit(
        const pb::OnPositionProfitRequest& req,
        std::function<void(const pb::OnPositionProfitData&)> callback);

    /// Subscribe to periodic position + pending-order ticket snapshots.
    /// @param callback  pb::OnPositionsAndPendingOrdersTicketsData.
    std::unique_ptr<AccountStream> onPositionsAndPendingOrdersTickets(
        const pb::OnPositionsAndPendingOrdersTicketsRequest& req,
        std::function<void(const pb::OnPositionsAndPendingOrdersTicketsData&)> callback);
#pragma endregion

    struct Impl;
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace metarpc
