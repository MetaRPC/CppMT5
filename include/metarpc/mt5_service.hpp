#pragma once
// MetaRPC CppMT5 — Layer 2 of 3: MT5Service (facade).
//
// MT5Service owns an MT5Account (Layer 1) and gives you a comfortable, protobuf-free API:
// friendly C++ structs instead of protobuf messages, plain arguments instead of hand-built
// *Request objects, and automatic UTC<->broker-server time conversion. This is the layer
// most application code should use directly. MT5Sugar (Layer 3) adds one-line conveniences
// on top of this.
//
// This header deliberately does NOT include the generated protobuf headers — it forward-
// declares MT5Account — so strategy code that includes it compiles fast and only sees the
// friendly types below. Reach the low-level layer with account() when you need a raw RPC.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "metarpc/mt5_common.hpp"   // ApiError, TransportError, ClientOptions

namespace metarpc {

class MT5Account;   // Layer 1 — forward-declared to keep protobuf out of this header

#pragma region Enums
// ======================================================================================
// Enums (friendly mirrors of the protobuf/MQL5 values)
// ======================================================================================

/// Account trade mode as reported by AccountSummary.
enum class TradeMode { Demo = 0, Contest = 1, Real = 2 };

/// MQL5 ENUM_TIMEFRAMES raw values — exactly what PriceHistory expects on the wire.
enum class Timeframe : int {
    M1 = 1, M2 = 2, M3 = 3, M4 = 4, M5 = 5, M6 = 6, M10 = 10, M12 = 12, M15 = 15, M20 = 20, M30 = 30,
    H1 = 16385, H2 = 16386, H3 = 16387, H4 = 16388, H6 = 16390, H8 = 16392, H12 = 16396,
    D1 = 16408, W1 = 32769, MN1 = 49153
};
/// Length of one bar of this timeframe, in seconds.
int timeframeSeconds(Timeframe tf);
/// Short timeframe name, e.g. "M15", "H1". Returns "?" for an unknown value.
const char* timeframeName(Timeframe tf);

/// Order direction / type for orderSend(); values match TMT5_ENUM_ORDER_TYPE on the wire.
enum class TradeAction {
    Buy = 0, Sell = 1, BuyLimit = 2, SellLimit = 3,
    BuyStop = 4, SellStop = 5, BuyStopLimit = 6, SellStopLimit = 7
};
#pragma endregion

#pragma region Data structs
// ======================================================================================
// Friendly data structs (what the facade returns / accepts)
// ======================================================================================

/// Snapshot of the trading account, from accountSummary().
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
    int64_t serverTime{0};          ///< unix seconds, broker server clock (raw, NOT converted)
    int64_t serverTimeUtc{0};       ///< the same instant in UTC
    int64_t utcShiftMinutes{0};     ///< server clock minus UTC, in minutes
    std::string server;             ///< mtClusterName used to connect
};

/// A single quote, from symbolInfoTick() or the tick stream. Times are UTC when the client
/// runs with utcTimes = true (the default).
struct Tick {
    std::string symbol;
    int64_t time{0};        ///< unix seconds
    int64_t timeMsc{0};     ///< unix milliseconds
    double bid{0};
    double ask{0};
    double last{0};
    double volume{0};
    uint32_t flags{0};
    bool twoSided() const { return bid > 0 && ask > 0 && ask > bid; }
};
using Quote = Tick;  ///< legacy alias

/// Full symbol specification, from symbolParams()/symbolParamsMany().
struct SymbolInfo {
    std::string name;
    std::string description;
    std::string path;
    std::string currencyBase;
    std::string currencyProfit;
    std::string currencyMargin;
    int digits{0};
    double point{0};
    double bid{0};              ///< from the bulk reply — may be stale; prefer symbolInfoTick()
    double ask{0};
    int spread{0};
    double contractSize{0};
    double tickSize{0};
    double tickValue{0};
    double volumeMin{0};
    double volumeMax{0};
    double volumeStep{0};
    double volumeLimit{0};
    int stopsLevel{0};          ///< points
    int freezeLevel{0};         ///< points
    int tradeMode{0};           ///< BMT5_ENUM_SYMBOL_TRADE_MODE: 0 DISABLED,1 LONGONLY,2 SHORTONLY,3 CLOSEONLY,4 FULL
    int tradeCalcMode{0};
    int tradeExeMode{0};
    int swapMode{0};
    double swapLong{0};
    double swapShort{0};
    std::vector<int> fillingModes;  ///< BMT5_ENUM_ORDER_TYPE_FILLING values the broker allows
    bool visible{false};
    bool selected{false};
    bool exist{false};
    int64_t time{0};            ///< last quote time, unix seconds
};

/// One OHLC bar, from priceHistory()/lastBars().
struct Bar {
    int64_t time{0};        ///< bar open time, unix seconds
    double open{0};
    double high{0};
    double low{0};
    double close{0};
    int64_t tickVolume{0};
    int64_t realVolume{0};
    int spread{0};
};

/// What you pass to orderSend(). Only the fields relevant to the action need to be set.
struct OrderRequest {
    std::string symbol;
    TradeAction action{TradeAction::Buy};
    double volume{0};
    double price{0};            ///< 0 = market (for Buy/Sell); required for pendings
    double stopLoss{0};
    double takeProfit{0};
    double stopLimit{0};        ///< for *StopLimit pendings
    int slippage{10};           ///< points
    int64_t magic{0};           ///< sent as expert_id; shows up as magic_number on positions/deals
    std::string comment;
    int64_t expiration{0};      ///< unix seconds; 0 = GTC
};

/// Result of a trade request (orderSend/Modify/Close).
struct OrderResult {
    uint32_t retcode{0};        ///< MQL5 trade retcode: 10009 DONE, 10008 PLACED, …
    int64_t ticket{0};          ///< == order
    int64_t deal{0};
    int64_t order{0};
    double volume{0};
    double price{0};
    double bid{0};
    double ask{0};
    std::string comment;
    std::string retcodeName;        ///< e.g. "TRADE_RETCODE_DONE"
    std::string retcodeDescription;
    uint32_t requestId{0};
    /// True for the "accepted" retcodes (done / placed / done-partial).
    bool ok() const { return retcode == 10009 || retcode == 10008 || retcode == 10010; }
};

/// An open position, from openedOrders()/positions().
struct Position {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                ///< 0 = BUY, 1 = SELL (BMT5_ENUM_POSITION_TYPE)
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
    int64_t identifier{0};      ///< POSITION_IDENTIFIER
    int reason{0};
    bool isBuy() const { return type == 0; }
};

/// A pending order, from openedOrders().
struct PendingOrder {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                ///< BMT5_ENUM_ORDER_TYPE
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

/// Everything live on the account right now.
struct OpenedOrders {
    std::vector<Position> positions;
    std::vector<PendingOrder> orders;
};

/// A single historical deal (execution), part of a HistoryItem.
struct Deal {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                ///< BMT5_ENUM_DEAL_TYPE (0 BUY, 1 SELL, 2 BALANCE, …)
    int entry{0};               ///< BMT5_ENUM_DEAL_ENTRY_TYPE (0 IN, 1 OUT, 2 INOUT, 3 OUT_BY)
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

/// A single historical order, part of a HistoryItem.
struct HistoryOrder {
    int64_t ticket{0};
    std::string symbol;
    int type{0};                ///< BMT5_ENUM_ORDER_TYPE
    int state{0};               ///< BMT5_ENUM_ORDER_STATE
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

/// One entry of orderHistory(): an order, a deal, or both (each flagged by has*).
struct HistoryItem {
    HistoryOrder order;
    bool hasOrder{false};
    Deal deal;
    bool hasDeal{false};
};

/// One closed position, from positionsHistory().
struct PositionHistory {
    int64_t ticket{0};
    std::string symbol;
    int orderType{0};           ///< AH_ENUM_POSITIONS_HISTORY_ORDER_TYPE
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

// --- small result structs for the extra unary calls ---------------------------------------

/// Margin rates for a symbol/order type, from symbolInfoMarginRate().
struct MarginRate {
    double initial{0};          ///< initial margin rate
    double maintenance{0};      ///< maintenance margin rate
};

/// A trading-session window, from symbolInfoSessionQuote()/SessionTrade(). Times are UTC
/// when the client runs with utcTimes = true.
struct SessionTime {
    int64_t from{0};            ///< session start, unix seconds
    int64_t to{0};              ///< session end, unix seconds
};

/// Result of orderCheck(): a dry-run validation of a trade request (MQL5 OrderCheck).
struct OrderCheckResult {
    uint32_t retcode{0};        ///< MQL5 return code (0 / 10009 = would succeed)
    double balance{0};          ///< balance after the hypothetical deal
    double equity{0};           ///< equity after the deal
    double profit{0};           ///< floating profit after the deal
    double margin{0};           ///< margin the order would use
    double freeMargin{0};       ///< free margin that would remain
    double marginLevel{0};      ///< resulting margin level, %
    std::string comment;
    bool ok() const { return retcode == 0 || retcode == 10009; }
};

/// One Depth-of-Market level, from marketBookGet().
struct BookEntry {
    int type{0};                ///< MrpcEnumBookType: 0 SELL, 1 BUY, 2 SELL_MARKET, 3 BUY_MARKET
    double price{0};
    int64_t volume{0};
    double volumeReal{0};
};

/// Tick value + sizing for a symbol, from tickValueWithSize().
struct TickValueSize {
    std::string symbol;
    double tickValue{0};        ///< tick value in the account currency
    double tickValueProfit{0};
    double tickValueLoss{0};
    double tickSize{0};
    double contractSize{0};
};

/// Ticket lists, from openedOrdersTickets().
struct OpenedTickets {
    std::vector<int64_t> positionTickets;
    std::vector<int64_t> orderTickets;
};
#pragma endregion

#pragma region Streaming event structs
// ======================================================================================
// Friendly projections of the On* subscription payloads (times normalised to UTC).
// ======================================================================================

/// Account snapshot carried by every trade/profit event (OnEventAccountInfo).
struct EventAccountInfo {
    int64_t login{0};
    double balance{0};
    double credit{0};
    double equity{0};
    double margin{0};
    double freeMargin{0};
    double marginLevel{0};
    double profit{0};
};

/// A pending/active order as reported by OnTrade (OnTradeOrderInfo).
struct TradeOrderInfo {
    uint32_t index{0};
    int64_t ticket{0};
    std::string symbol;
    int type{0};                ///< order type
    int state{0};
    int typeFilling{0};
    int timeType{0};
    int reason{0};
    double volumeInitial{0};
    double volumeCurrent{0};
    double priceOpen{0};
    double priceCurrent{0};
    double stopLoss{0};
    double takeProfit{0};
    double stopLimit{0};
    int64_t magic{0};
    int64_t positionId{0};
    int64_t positionById{0};
    std::string comment;
    int64_t setupTime{0};       ///< UTC seconds
    int64_t doneTime{0};        ///< UTC seconds (0 for active orders)
    int64_t expiration{0};      ///< UTC seconds
};

/// A position as reported by OnTrade (OnTradePositionInfo).
struct TradePositionInfo {
    uint32_t index{0};
    int64_t ticket{0};
    std::string symbol;
    int type{0};                ///< 0 BUY, 1 SELL
    int reason{0};
    double volume{0};
    double priceOpen{0};
    double priceCurrent{0};
    double stopLoss{0};
    double takeProfit{0};
    double profit{0};
    double swap{0};
    int64_t magic{0};
    bool fromPendingOrder{false};
    std::string comment;
    int64_t positionTime{0};    ///< UTC seconds
    int64_t lastUpdateTime{0};  ///< UTC seconds
};

/// A historical order as reported by OnTrade (OnTradeHistoryOrderInfo).
struct TradeHistoryOrderInfo {
    uint32_t index{0};
    int64_t ticket{0};
    std::string symbol;
    int type{0};
    int state{0};
    int typeFilling{0};
    int timeType{0};
    int reason{0};
    double volumeInitial{0};
    double volumeCurrent{0};
    double priceOpen{0};
    double priceCurrent{0};
    double stopLoss{0};
    double takeProfit{0};
    double stopLimit{0};
    int64_t magic{0};
    int64_t positionId{0};
    int64_t positionBy{0};
    std::string comment;
    int64_t setupTime{0};       ///< UTC seconds
    int64_t doneTime{0};        ///< UTC seconds
    int64_t expiration{0};      ///< UTC seconds
};

/// A historical deal as reported by OnTrade (OnTradeHistoryDealInfo).
struct TradeHistoryDealInfo {
    uint32_t index{0};
    int64_t ticket{0};
    std::string symbol;
    int type{0};
    int entry{0};
    int reason{0};
    double volume{0};
    double price{0};
    double profit{0};
    double commission{0};
    double swap{0};
    double fee{0};
    double stopLoss{0};
    double takeProfit{0};
    int64_t magic{0};
    int64_t orderTicket{0};
    int64_t positionId{0};
    std::string comment;
    int64_t time{0};            ///< UTC seconds
};

/// before/after pair for an updated entity in OnTrade.
template <class T> struct Change { T previous; T current; };

/// The full payload of an OnTrade event: which groups changed, plus the account snapshot.
struct TradeEvent {
    int type{0};                ///< MT5_SUB_ENUM_EVENT_GROUP_TYPE (0 OrderProfit, 1 OrderUpdate)
    std::string terminalGuid;
    EventAccountInfo account;
    std::vector<TradeOrderInfo> newOrders, disappearedOrders;
    std::vector<Change<TradeOrderInfo>> stateChangedOrders;
    std::vector<TradeHistoryOrderInfo> newHistoryOrders, disappearedHistoryOrders;
    std::vector<Change<TradeHistoryOrderInfo>> updatedHistoryOrders;
    std::vector<TradeHistoryDealInfo> newHistoryDeals, disappearedHistoryDeals;
    std::vector<Change<TradeHistoryDealInfo>> updatedHistoryDeals;
    std::vector<TradePositionInfo> newPositions, disappearedPositions;
    std::vector<Change<TradePositionInfo>> updatedPositions;
};

/// MQL5 MqlTradeTransaction, as carried by OnTradeTransaction.
struct TradeTransaction {
    int type{0};
    int64_t dealTicket{0};
    int64_t orderTicket{0};
    int64_t positionTicket{0};
    int64_t positionByOpposite{0};
    std::string symbol;
    int dealType{0};
    int orderType{0};
    int orderState{0};
    int orderTimeType{0};
    double price{0};
    double priceTriggerStopLimit{0};
    double priceStopLoss{0};
    double priceTakeProfit{0};
    double volume{0};
    int64_t orderExpiration{0}; ///< UTC seconds
};

/// MQL5 MqlTradeRequest, as carried by OnTradeTransaction.
struct TradeRequestInfo {
    int operationType{0};       ///< trade_operation_type
    int orderType{0};
    int typeFilling{0};
    int timeType{0};
    int64_t magic{0};
    int64_t orderTicket{0};
    int64_t positionTicket{0};
    int64_t positionByOpposite{0};
    std::string symbol;
    double volumeLots{0};       ///< requested_deal_volume_lots
    double price{0};
    double stopLoss{0};
    double takeProfit{0};
    double stopLimit{0};
    uint64_t deviation{0};
    std::string comment;
    int64_t orderExpiration{0}; ///< UTC seconds
};

/// MQL5 MqlTradeResult, as carried by OnTradeTransaction.
struct TradeResultInfo {
    uint32_t retcode{0};        ///< trade_return_code
    int retcodeInt{0};          ///< trade_return_int_code
    int64_t dealTicket{0};
    int64_t orderTicket{0};
    double dealVolume{0};
    double dealPrice{0};
    double bid{0};
    double ask{0};
    std::string comment;        ///< broker_comment_to_operation
    uint32_t requestId{0};      ///< terminal_dispatch_request_id
    int returnCodeExternal{0};
};

/// The full payload of an OnTradeTransaction event.
struct TransactionEvent {
    int type{0};                ///< MT5_SUB_ENUM_EVENT_GROUP_TYPE
    std::string terminalGuid;
    EventAccountInfo account;
    TradeTransaction transaction;
    TradeRequestInfo request;
    TradeResultInfo result;
};

/// One position in an OnPositionProfit snapshot (OnPositionProfitPositionInfo).
struct PositionProfitInfo {
    uint32_t index{0};
    int64_t ticket{0};
    std::string symbol;
    double profit{0};
};

/// The full payload of an OnPositionProfit event.
struct PositionProfitEvent {
    int type{0};                ///< MT5_SUB_ENUM_EVENT_GROUP_TYPE
    std::string terminalGuid;
    EventAccountInfo account;
    std::vector<PositionProfitInfo> newPositions, updatedPositions, deletedPositions;
};

/// The full payload of an OnPositionsAndPendingOrdersTickets snapshot.
struct TicketsSnapshot {
    std::string terminalGuid;
    std::vector<int64_t> positionTickets;
    std::vector<int64_t> pendingOrderTickets;
    int64_t serverTime{0};      ///< UTC seconds
};
#pragma endregion

#pragma region Streaming types
// ======================================================================================
// Friendly tick subscription — delivers converted Tick objects (times in UTC).
// ======================================================================================

/// Handle to a running friendly subscription created by MT5Service (ticks, trades, position
/// profit, …). Wraps the low-level stream and hands your callback ready-to-use friendly structs.
/// Keep it alive to keep receiving; destroy it or call stop() to cancel. One handle type serves
/// every MT5Service stream.
class ServiceStream {
public:
    ~ServiceStream();
    void stop();                    ///< cancel the stream and join the reader thread
    bool running() const;           ///< false once the stream ended or was cancelled
    uint64_t count() const;         ///< payloads delivered so far
    std::string lastError() const;  ///< the error that ended the stream, or "" if clean
    struct Impl;
private:
    friend class MT5Service;
    ServiceStream();
    std::unique_ptr<Impl> m_impl;
};
#pragma endregion

// ======================================================================================
// MT5Service — Layer 2 (friendly facade over MT5Account)
// ======================================================================================
class MT5Service {
public:
    /// Construct the facade and the MT5Account it owns. Nothing connects yet — call connect().
    /// @param host/port/apiKey/name  same meaning as the MT5Account constructor.
    /// @param utcTimes  when true (default), every time this facade returns is UTC and every
    ///                  time you pass in is treated as UTC; set false to work in raw server time.
    explicit MT5Service(const std::string& host = "mt5.mrpc.pro", int port = 443,
                        const std::string& apiKey = "", const std::string& name = "",
                        bool utcTimes = true);
    ~MT5Service();
    MT5Service(const MT5Service&) = delete;
    MT5Service& operator=(const MT5Service&) = delete;

    /// The underlying low-level layer, for raw protobuf RPCs not surfaced here.
    MT5Account& account();

#pragma region Connection
    // --- connection ---------------------------------------------------------------------

    /// Connect (ConnectEx) by broker server name; stores credentials for auto-reconnect and
    /// blocks until the terminal is ready. @throws ApiError / TransportError. See MT5Account::connect.
    void connect(int64_t login, const std::string& password, const std::string& server,
                 const std::string& name = "", int timeoutSeconds = 120);
    void reconnect();                           ///< re-issue ConnectEx with the stored credentials
    bool isConnected() const;                   ///< true after a successful connect/reconnect
    std::string terminalGuid() const;           ///< the terminal id the gateway returned
    void disconnect(const std::string& reason = "");  ///< Disconnect RPC — STOPS the terminal
    void close();                               ///< drop the channel only; the terminal keeps running
    ClientOptions& options();                   ///< mutable transport/timeout options
#pragma endregion

#pragma region Time
    // --- time ---------------------------------------------------------------------------

    int64_t serverOffsetSeconds();              ///< broker clock minus UTC (seconds), cached
    int64_t toUtc(int64_t serverTime);          ///< raw broker server time -> UTC
    int64_t toServerTime(int64_t utc);          ///< UTC -> raw broker server time
#pragma endregion

#pragma region Account
    // --- account ------------------------------------------------------------------------

    /// Full account snapshot as a friendly AccountInfo (times filled in UTC and raw).
    AccountInfo accountSummary();
    AccountInfo getAccountInfo() { return accountSummary(); }  ///< legacy alias
    /// One double account property by AccountInfoDoublePropertyType id (0 BALANCE … 13).
    double accountInfoDouble(int propertyId);
    /// One integer account property by AccountInfoIntegerPropertyType id.
    int64_t accountInfoInteger(int propertyId);
    /// One string account property by AccountInfoStringPropertyType id.
    std::string accountInfoString(int propertyId);
#pragma endregion

#pragma region Symbols / quotes
    // --- symbols / quotes ---------------------------------------------------------------

    bool symbolSelect(const std::string& symbol, bool select = true);  ///< add/remove from Market Watch
    bool symbolExist(const std::string& symbol);                       ///< does the broker know it
    bool symbolIsSynchronized(const std::string& symbol);              ///< is its data synced
    std::vector<std::string> symbolList();                             ///< all symbols on the terminal
    /// Latest tick for a symbol (times in UTC by default).
    Tick symbolInfoTick(const std::string& symbol);
    /// Full parameters for one symbol (exact-name lookup). @throws ApiError if not found.
    SymbolInfo symbolParams(const std::string& symbol);
    /// Full parameters for many symbols; empty filter = every symbol.
    std::vector<SymbolInfo> symbolParamsMany(const std::string& nameFilter = "");
    /// Raw property getters; propertyId is the PROTO enum (SymbolInfoDoubleProperty, …).
    /// Prefer symbolParams() unless you need a single field.
    double symbolInfoDouble(const std::string& symbol, int propertyId);
    int64_t symbolInfoInteger(const std::string& symbol, int propertyId);
    std::string symbolInfoString(const std::string& symbol, int propertyId);

    /// Count of symbols — all, or just the Market Watch selection.
    int symbolsTotal(bool selectedOnly = false);
    /// Symbol name by index (pair with symbolsTotal() to enumerate).
    std::string symbolName(int index, bool selectedOnly = false);
    /// Initial/maintenance margin rates for a symbol and order type.
    MarginRate symbolInfoMarginRate(const std::string& symbol, int orderType);
    /// Quote-session window for a symbol on a weekday (0=Sun..6=Sat) + session index.
    SessionTime symbolInfoSessionQuote(const std::string& symbol, int dayOfWeek, uint32_t sessionIndex = 0);
    /// Trade-session window for a symbol on a weekday (0=Sun..6=Sat) + session index.
    SessionTime symbolInfoSessionTrade(const std::string& symbol, int dayOfWeek, uint32_t sessionIndex = 0);
    /// Tick value + contract size for several symbols at once (for P&L / position sizing).
    std::vector<TickValueSize> tickValueWithSize(const std::vector<std::string>& symbols);
#pragma endregion

#pragma region Market depth (DOM)
    // --- market depth (DOM) -------------------------------------------------------------

    bool marketBookAdd(const std::string& symbol);                 ///< subscribe to a symbol's DOM
    std::vector<BookEntry> marketBookGet(const std::string& symbol);  ///< current DOM snapshot
    bool marketBookRelease(const std::string& symbol);             ///< unsubscribe from a symbol's DOM
#pragma endregion

#pragma region Bars
    // --- bars ---------------------------------------------------------------------------

    /// OHLC bars over [fromUnix, toUnix] (UTC when utcTimes). maxBars 0 = no client limit.
    std::vector<Bar> priceHistory(const std::string& symbol, Timeframe tf, int64_t fromUnix,
                                  int64_t toUnix, int maxBars = 0);
    /// The newest `count` bars for a symbol/timeframe.
    std::vector<Bar> lastBars(const std::string& symbol, Timeframe tf, int count);
#pragma endregion

#pragma region Trading
    // --- trading ------------------------------------------------------------------------

    /// Place a market or pending order. Inspect OrderResult::retcode / ok().
    OrderResult orderSend(const OrderRequest& request);
    /// Modify SL/TP (and price/stop-limit for pendings) of an order/position by ticket.
    OrderResult orderModify(int64_t ticket, double stopLoss, double takeProfit,
                            double price = 0, double stopLimit = 0);
    /// Close a position (volume 0 = the whole position) or delete a pending order by ticket.
    OrderResult orderClose(int64_t ticket, double volume = 0, int slippage = 10);
    /// All open positions and pending orders.
    OpenedOrders openedOrders();
    /// Open positions, optionally filtered by magic number (-1 = all).
    std::vector<Position> positions(int64_t magic = -1);
    /// Number of open positions.
    int positionsTotal();
    /// Just the tickets of live positions and pending orders (lighter than openedOrders()).
    OpenedTickets openedOrdersTickets();
    /// Dry-run validate an order request (margin, free margin, retcode) before sending it.
    OrderCheckResult orderCheck(const OrderRequest& request);
    /// Margin required to open a given order (symbol, order type, volume, price).
    double orderCalcMargin(const std::string& symbol, int orderType, double volume, double price);
    /// Profit a price move would yield (symbol, order type, volume, open price, close price).
    double orderCalcProfit(const std::string& symbol, int orderType, double volume,
                           double openPrice, double closePrice);
#pragma endregion

#pragma region History
    // --- history ------------------------------------------------------------------------

    /// Closed orders and deals in [fromUnix, toUnix] (UTC when utcTimes).
    std::vector<HistoryItem> orderHistory(int64_t fromUnix, int64_t toUnix);
    /// Closed positions in [fromUnix, toUnix] (UTC when utcTimes).
    std::vector<PositionHistory> positionsHistory(int64_t fromUnix, int64_t toUnix);
#pragma endregion

#pragma region Streaming
    // --- streaming ----------------------------------------------------------------------
    // Every subscribe*/stream* method converts the raw payload to friendly structs (times in
    // UTC) and delivers them to `callback` on a background thread. Keep the returned handle
    // alive to keep receiving; destroy it or call stop() to cancel.

    /// Live ticks for the given symbols.
    std::unique_ptr<ServiceStream> subscribeTicks(
        const std::vector<std::string>& symbols, std::function<void(const Tick&)> callback);
    /// Trade events (orders/positions/deals appearing, changing or closing).
    std::unique_ptr<ServiceStream> streamTradeUpdates(std::function<void(const TradeEvent&)> callback);
    /// Detailed trade-transaction log (every transaction/request/result).
    std::unique_ptr<ServiceStream> streamTransactions(std::function<void(const TransactionEvent&)> callback);
    /// Periodic position-profit snapshots.
    std::unique_ptr<ServiceStream> streamPositionProfits(std::function<void(const PositionProfitEvent&)> callback);
    /// Periodic position + pending-order ticket snapshots.
    std::unique_ptr<ServiceStream> streamOpenedTickets(std::function<void(const TicketsSnapshot&)> callback);
#pragma endregion

    struct Impl;
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace metarpc
