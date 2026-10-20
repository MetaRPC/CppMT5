#pragma once
// MetaRPC CppMT5 — C++17 client for the MetaRPC MT5 gRPC gateway (mt5.mrpc.pro:443).
//
// The public surface deliberately hides protobuf/gRPC types (pimpl) so strategy code
// compiles fast and never has to include the 11 MB of generated headers in package/.
// Every call is synchronous; every failure is an exception:
//   - ApiError       : the gateway/terminal answered with an error (code + message, e.g.
//                      "CONNECT_EX_INVALID_LOGIN_ERROR", "TERMINAL_INSTANCE_NOT_FOUND").
//   - TransportError : gRPC transport failure after the retry budget was spent.
// Transient gRPC failures (UNAVAILABLE, DEADLINE_EXCEEDED) are retried with exponential
// back-off; TERMINAL_INSTANCE_NOT_FOUND triggers an automatic ConnectEx re-attach and a
// retry. INVALID_LOGIN is never retried.

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace metarpc {

class ApiError : public std::runtime_error {
public:
    ApiError(const std::string& code, const std::string& message, int mqlErrorCode = 0,
             int mqlTradeCode = 0, const std::string& mqlTradeDescription = "",
             const std::string& where = "")
        : std::runtime_error((where.empty() ? "" : where + ": ") + code + ": " + message),
          code(code), message(message), mqlErrorCode(mqlErrorCode), mqlTradeCode(mqlTradeCode),
          mqlTradeDescription(mqlTradeDescription), where(where) {}
    std::string code;           // gateway error code, e.g. CONNECT_EX_INVALID_LOGIN_ERROR
    std::string message;
    int mqlErrorCode;           // MQL5 GetLastError() style code (0 if none)
    int mqlTradeCode;           // MQL5 trade return code (0 if none)
    std::string mqlTradeDescription;
    std::string where;          // RPC name
    bool isInvalidLogin() const {
        return code.find("INVALID_LOGIN") != std::string::npos || code.find("WRONG_PASSWORD") != std::string::npos;
    }
    bool isTerminalNotFound() const {
        return code == "TERMINAL_INSTANCE_NOT_FOUND" || code == "TERMINAL_REGISTRY_TERMINAL_NOT_FOUND";
    }
};

class TransportError : public std::runtime_error {
public:
    TransportError(int grpcCode, const std::string& message)
        : std::runtime_error(message), grpcCode(grpcCode) {}
    int grpcCode;               // grpc::StatusCode as int (14 = UNAVAILABLE, 4 = DEADLINE_EXCEEDED)
};

enum class TradeMode { Demo = 0, Contest = 1, Real = 2 };

struct AccountInfo {
    int64_t login{0};
    std::string currency;
    double balance{0};
    double equity{0};
    double credit{0};
    int64_t leverage{0};
    std::string name;
    std::string company;
    TradeMode tradeMode{TradeMode::Demo};
    int64_t serverTime{0};          // unix seconds, broker server clock (raw, NOT converted)
    int64_t serverTimeUtc{0};       // the same instant in UTC
    int64_t utcShiftMinutes{0};     // server clock minus UTC, minutes (FXTM: 180)
    std::string server;             // mtClusterName used to connect
};

struct Tick {
    std::string symbol;
    int64_t time{0};        // unix seconds
    int64_t timeMsc{0};     // unix milliseconds
    double bid{0};
    double ask{0};
    double last{0};
    double volume{0};
    uint32_t flags{0};
    bool twoSided() const { return bid > 0 && ask > 0 && ask > bid; }
};

// Legacy alias kept for the original public API.
using Quote = Tick;

struct SymbolInfo {
    std::string name;
    std::string description;
    std::string path;
    std::string currencyBase;
    std::string currencyProfit;
    std::string currencyMargin;
    int digits{0};
    double point{0};
    double bid{0};              // from the bulk reply — may be stale; prefer symbolInfoTick()
    double ask{0};
    int spread{0};
    double contractSize{0};
    double tickSize{0};
    double tickValue{0};
    double volumeMin{0};
    double volumeMax{0};
    double volumeStep{0};
    double volumeLimit{0};
    int stopsLevel{0};          // points
    int freezeLevel{0};         // points
    int tradeMode{0};           // BMT5_ENUM_SYMBOL_TRADE_MODE: 0 DISABLED,1 LONGONLY,2 SHORTONLY,3 CLOSEONLY,4 FULL
    int tradeCalcMode{0};
    int tradeExeMode{0};
    int swapMode{0};
    double swapLong{0};
    double swapShort{0};
    std::vector<int> fillingModes;  // BMT5_ENUM_ORDER_TYPE_FILLING values the broker allows
    bool visible{false};
    bool selected{false};
    bool exist{false};
    int64_t time{0};            // last quote time, unix seconds
};

// MQL5 ENUM_TIMEFRAMES raw values — this is exactly what PriceHistory expects.
enum class Timeframe : int {
    M1 = 1, M2 = 2, M3 = 3, M4 = 4, M5 = 5, M6 = 6, M10 = 10, M12 = 12, M15 = 15, M20 = 20, M30 = 30,
    H1 = 16385, H2 = 16386, H3 = 16387, H4 = 16388, H6 = 16390, H8 = 16392, H12 = 16396,
    D1 = 16408, W1 = 32769, MN1 = 49153
};
int timeframeSeconds(Timeframe tf);
const char* timeframeName(Timeframe tf);

struct Bar {
    int64_t time{0};        // bar open time, unix seconds
    double open{0};
    double high{0};
    double low{0};
    double close{0};
    int64_t tickVolume{0};
    int64_t realVolume{0};
    int spread{0};
};

// Values equal TMT5_ENUM_ORDER_TYPE on the wire.
enum class TradeAction {
    Buy = 0,
    Sell = 1,
    BuyLimit = 2,
    SellLimit = 3,
    BuyStop = 4,
    SellStop = 5,
    BuyStopLimit = 6,
    SellStopLimit = 7
};

struct OrderRequest {
    std::string symbol;
    TradeAction action{TradeAction::Buy};
    double volume{0};
    double price{0};            // 0 = market (for Buy/Sell); required for pendings
    double stopLoss{0};
    double takeProfit{0};
    double stopLimit{0};        // for *StopLimit pendings
    int slippage{10};           // points
    int64_t magic{0};           // sent as expert_id; shows up as magic_number on positions/deals
    std::string comment;
    int64_t expiration{0};      // unix seconds; 0 = GTC
};

struct OrderResult {
    uint32_t retcode{0};        // MQL5 trade retcode: 10009 TRADE_RETCODE_DONE, 10008 PLACED, ...
    int64_t ticket{0};          // == order
    int64_t deal{0};
    int64_t order{0};
    double volume{0};
    double price{0};
    double bid{0};
    double ask{0};
    std::string comment;
    std::string retcodeName;        // e.g. "TRADE_RETCODE_DONE"
    std::string retcodeDescription;
    uint32_t requestId{0};
    bool ok() const { return retcode == 10009 || retcode == 10008 || retcode == 10010; }
};

struct Position {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                // 0 = BUY, 1 = SELL (BMT5_ENUM_POSITION_TYPE)
    double volume{0};
    double priceOpen{0};
    double priceCurrent{0};
    double stopLoss{0};
    double takeProfit{0};
    double profit{0};
    double swap{0};
    double commission{0};
    int64_t magic{0};
    std::string comment;
    int64_t openTime{0};
    int64_t identifier{0};      // POSITION_IDENTIFIER
    int reason{0};
    bool isBuy() const { return type == 0; }
};

struct PendingOrder {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                // BMT5_ENUM_ORDER_TYPE
    int state{0};
    double volumeInitial{0};
    double volumeCurrent{0};
    double priceOpen{0};
    double priceCurrent{0};
    double stopLoss{0};
    double takeProfit{0};
    double stopLimit{0};
    int64_t magic{0};
    std::string comment;
    int64_t timeSetup{0};
    int64_t timeExpiration{0};
    int64_t positionId{0};
};

struct OpenedOrders {
    std::vector<Position> positions;
    std::vector<PendingOrder> orders;
};

struct Deal {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                // BMT5_ENUM_DEAL_TYPE (0 BUY, 1 SELL, 2 BALANCE, ...)
    int entry{0};               // BMT5_ENUM_DEAL_ENTRY_TYPE (0 IN, 1 OUT, 2 INOUT, 3 OUT_BY)
    double volume{0};
    double price{0};
    double profit{0};
    double commission{0};
    double swap{0};
    double fee{0};
    double stopLoss{0};
    double takeProfit{0};
    int64_t positionId{0};
    int64_t time{0};
    std::string comment;
    int reason{0};
};

struct HistoryOrder {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                // BMT5_ENUM_ORDER_TYPE
    int state{0};               // BMT5_ENUM_ORDER_STATE
    double priceOpen{0};
    double priceCurrent{0};
    double stopLoss{0};
    double takeProfit{0};
    double volumeInitial{0};
    double volumeCurrent{0};
    int64_t magic{0};
    std::string comment;
    int64_t setupTime{0};
    int64_t doneTime{0};
    int64_t positionId{0};
};

struct HistoryItem {
    HistoryOrder order;
    bool hasOrder{false};
    Deal deal;
    bool hasDeal{false};
};

struct PositionHistory {
    int64_t ticket{0};
    std::string symbol;
    int orderType{0};           // AH_ENUM_POSITIONS_HISTORY_ORDER_TYPE
    double volume{0};
    double openPrice{0};
    double closePrice{0};
    double stopLoss{0};
    double takeProfit{0};
    double profit{0};
    double swap{0};
    double commission{0};
    double fee{0};
    int64_t openTime{0};
    int64_t closeTime{0};
    int64_t magic{0};
    std::string comment;
};

struct ClientOptions {
    int unaryTimeoutMs{30000};          // deadline for ordinary RPCs
    int connectTimeoutMs{200000};       // deadline for ConnectEx (terminal start can take > 1 min)
    int tradeTimeoutMs{60000};          // deadline for OrderSend/Modify/Close
    int maxRetries{5};                  // transient-failure retry budget per call
    int maxBackoffMs{5000};
    int maxReceiveMessageBytes{64 * 1024 * 1024};
    std::string rootCertsPath;          // PEM bundle; empty = autodetect (/etc/ssl/certs/ca-certificates.crt)
    bool verbose{false};                // log retries/reconnects to stderr (also MRPC_VERBOSE=1)
    // The gateway speaks BROKER SERVER TIME everywhere (tick.time, bar.time, deal/position times,
    // PriceHistory/OrderHistory/PositionsHistory windows). With utcTimes=true (default) the SDK
    // converts: every time you pass in is UTC, every time you get back is UTC. The offset comes
    // from AccountSummary.utc_timezone_server_time_shift_minutes and is cached per client.
    // Set false to see raw server times and pass raw server-time windows.
    bool utcTimes{true};
};

// A running OnSymbolTick subscription. Ticks are delivered on a background thread.
class TickStream {
public:
    ~TickStream();
    void stop();                        // cancels the stream and joins the reader thread
    bool running() const;
    uint64_t count() const;             // ticks delivered so far
    std::string lastError() const;      // empty if the stream ended cleanly / was cancelled
    struct Impl;
private:
    friend class MT5Client;
    TickStream();
    std::unique_ptr<Impl> m_impl;
};

class MT5Client {
public:
    explicit MT5Client(const std::string& host = "mt5.mrpc.pro", int port = 443,
                       const std::string& apiKey = "", const std::string& name = "");
    ~MT5Client();
    MT5Client(const MT5Client&) = delete;
    MT5Client& operator=(const MT5Client&) = delete;

    // --- identity -----------------------------------------------------------------------
    static constexpr const char* NAMESPACE_DNS = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
    static constexpr const char* NAMESPACE_FLEET = "6f1c3b2e-7a4d-4c0e-9a1f-0000f1ee0001";
    // RFC 4122 UUID v5 (SHA-1) — the fleet convention for stable terminal ids.
    static std::string uuid5(const std::string& namespaceUuid, const std::string& name);
    // Legacy SHA-256 based id (login:password) kept for compatibility with older code.
    static std::string computeDeterministicId(int64_t login, const std::string& password);
    std::string getId(int64_t login, const std::string& password);   // legacy: computes + sets
    void setId(const std::string& id);          // set BEFORE connect(); stable per account
    const std::string& getId() const;
    void setApiKey(const std::string& apiKey);
    const std::string& getApiKey() const;
    void setName(const std::string& name);
    const std::string& getName() const;
    ClientOptions& options();

    // --- connection ---------------------------------------------------------------------
    // ConnectEx by broker server name (mtClusterName). Throws ApiError / TransportError.
    // baseChartSymbol is accepted for source compatibility only: the gateway removed the
    // base chart from ConnectExRequest (proto field 4 is reserved) and the value is ignored.
    void connect(int64_t login, const std::string& password, const std::string& server,
                 const std::string& name = "", const std::string& baseChartSymbol = "EURUSD",
                 int timeoutSeconds = 120);
    void reconnect();                   // re-issues ConnectEx with the stored credentials
    bool isConnected() const;
    std::string terminalGuid() const;   // id returned by the gateway (== getId() normally)
    // Sends the Disconnect RPC: the gateway STOPS the terminal. Do not call this from a
    // long-running strategy's shutdown unless you really want the terminal gone.
    void disconnect(const std::string& reason = "");
    void close();                       // drops the gRPC channel only; the terminal keeps running

    // --- time ---------------------------------------------------------------------------
    int64_t serverOffsetSeconds();              // broker clock minus UTC (cached; one AccountSummary)
    int64_t toUtc(int64_t serverTime);          // raw server time -> UTC
    int64_t toServerTime(int64_t utc);          // UTC -> raw server time

    // --- account ------------------------------------------------------------------------
    AccountInfo accountSummary();
    AccountInfo getAccountInfo() { return accountSummary(); }   // legacy name
    double accountInfoDouble(int propertyId);     // AccountInfoDoublePropertyType (0 BALANCE .. 13)
    int64_t accountInfoInteger(int propertyId);   // AccountInfoIntegerPropertyType
    std::string accountInfoString(int propertyId);// AccountInfoStringPropertyType

    // --- symbols / quotes ---------------------------------------------------------------
    bool symbolSelect(const std::string& symbol, bool select = true);
    bool symbolExist(const std::string& symbol);
    bool symbolIsSynchronized(const std::string& symbol);
    std::vector<std::string> symbolList();        // all symbols known to the terminal
    Tick symbolInfoTick(const std::string& symbol);
    SymbolInfo symbolParams(const std::string& symbol);               // exact-name lookup
    std::vector<SymbolInfo> symbolParamsMany(const std::string& nameFilter = "");
    // Raw property getters; the propertyId is the PROTO enum (SymbolInfoDoubleProperty etc.),
    // NOT the MQL5 value. Prefer symbolParams().
    double symbolInfoDouble(const std::string& symbol, int propertyId);
    int64_t symbolInfoInteger(const std::string& symbol, int propertyId);
    std::string symbolInfoString(const std::string& symbol, int propertyId);

    // --- bars ---------------------------------------------------------------------------
    std::vector<Bar> priceHistory(const std::string& symbol, Timeframe tf, int64_t fromUnix,
                                  int64_t toUnix, int maxBars = 0);
    std::vector<Bar> lastBars(const std::string& symbol, Timeframe tf, int count);

    // --- streaming ----------------------------------------------------------------------
    std::unique_ptr<TickStream> subscribeTicks(const std::vector<std::string>& symbols,
                                               std::function<void(const Tick&)> callback);
    // Legacy helper: blocks for durationMs delivering ticks, then unsubscribes.
    uint64_t subscribeQuotes(const std::vector<std::string>& symbols,
                             std::function<void(const Tick&)> callback, int durationMs);

    // --- trading ------------------------------------------------------------------------
    OrderResult orderSend(const OrderRequest& request);
    OrderResult orderModify(int64_t ticket, double stopLoss, double takeProfit,
                            double price = 0, double stopLimit = 0);
    // volume 0 = close the whole position (looked up via openedOrders()).
    OrderResult orderClose(int64_t ticket, double volume = 0, int slippage = 10);
    OpenedOrders openedOrders();
    std::vector<Position> positions(int64_t magic = -1);    // -1 = all
    int positionsTotal();

    // --- history ------------------------------------------------------------------------
    std::vector<HistoryItem> orderHistory(int64_t fromUnix, int64_t toUnix);
    std::vector<PositionHistory> positionsHistory(int64_t fromUnix, int64_t toUnix);

    struct Impl;
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace metarpc
