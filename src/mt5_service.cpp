// MetaRPC CppMT5 — Layer 2 (MT5Service) implementation: build requests, call MT5Account,
// convert protobuf payloads into the friendly structs, and shift broker-server times to/from UTC.
#include "metarpc/mt5_service.hpp"
#include "metarpc/mt5_account.hpp"

#include <google/protobuf/timestamp.pb.h>

#include <algorithm>

namespace metarpc {

// ======================================================================================
// Timeframe helpers (pure)
// ======================================================================================
#pragma region Timeframe helpers
// Seconds in one bar of this timeframe. Minutes map 1:1 (M1..M30), the H* block is offset by
// 16384, W1/MN1 are special — mirrors the MQL5 ENUM_TIMEFRAMES numbering.
int timeframeSeconds(Timeframe tf) {
    int v = (int)tf;
    if (v < 16385) return v * 60;          // M1..M30 — the raw value is the minute count
    if (v < 32769) return (v - 16384) * 3600;  // H1..H12 — low bits are the hour count
    if (v == 32769) return 7 * 86400;      // W1
    return 30 * 86400;                     // MN1 (approx. one month)
}

const char* timeframeName(Timeframe tf) {
    switch (tf) {
        case Timeframe::M1: return "M1"; case Timeframe::M2: return "M2"; case Timeframe::M3: return "M3";
        case Timeframe::M4: return "M4"; case Timeframe::M5: return "M5"; case Timeframe::M6: return "M6";
        case Timeframe::M10: return "M10"; case Timeframe::M12: return "M12"; case Timeframe::M15: return "M15";
        case Timeframe::M20: return "M20"; case Timeframe::M30: return "M30"; case Timeframe::H1: return "H1";
        case Timeframe::H2: return "H2"; case Timeframe::H3: return "H3"; case Timeframe::H4: return "H4";
        case Timeframe::H6: return "H6"; case Timeframe::H8: return "H8"; case Timeframe::H12: return "H12";
        case Timeframe::D1: return "D1"; case Timeframe::W1: return "W1"; case Timeframe::MN1: return "MN1";
    }
    return "?";
}
#pragma endregion

// ======================================================================================
// protobuf -> friendly struct converters (the heart of this layer)
// ======================================================================================
#pragma region Converters
namespace {

// Copy a MrpcMqlTick (the unary SymbolInfoTick payload) into a friendly Tick. volume_real is
// preferred over the integer volume; a missing seconds time is recovered from the ms field.
Tick tickFrom(const pb::MrpcMqlTick& t, const std::string& symbol) {
    Tick o; o.symbol = symbol; o.time = t.time(); o.timeMsc = t.time_msc(); o.bid = t.bid(); o.ask = t.ask();
    o.last = t.last(); o.volume = t.volume_real() > 0 ? t.volume_real() : (double)t.volume(); o.flags = t.flags();
    if (o.time == 0 && o.timeMsc > 0) o.time = o.timeMsc / 1000;
    return o;
}

// Same, for the streaming tick payload (which carries its own symbol and a Timestamp time).
Tick tickFrom(const pb::MrpcSubscriptionMqlTick& t) {
    Tick o; o.symbol = t.symbol(); o.time = t.time().seconds(); o.timeMsc = t.time_msc(); o.bid = t.bid(); o.ask = t.ask();
    o.last = t.last(); o.volume = t.volume_real() > 0 ? t.volume_real() : (double)t.volume(); o.flags = t.flags();
    if (o.time == 0 && o.timeMsc > 0) o.time = o.timeMsc / 1000;
    return o;
}

// Flatten one SymbolParameters entry into a SymbolInfo (prices, sizing, swaps, modes, flags).
SymbolInfo symbolFrom(const pb::SymbolParameters& p) {
    SymbolInfo s;
    s.name = p.name(); s.description = p.sym_description(); s.path = p.path();
    s.currencyBase = p.currency_base(); s.currencyProfit = p.currency_profit(); s.currencyMargin = p.currency_margin();
    s.digits = p.digits(); s.point = p.point(); s.bid = p.bid(); s.ask = p.ask(); s.spread = p.spread();
    s.contractSize = p.trade_contract_size(); s.tickSize = p.trade_tick_size(); s.tickValue = p.trade_tick_value();
    s.volumeMin = p.volume_min(); s.volumeMax = p.volume_max(); s.volumeStep = p.volume_step(); s.volumeLimit = p.volume_limit();
    s.stopsLevel = p.trade_stops_level(); s.freezeLevel = p.trade_freeze_level();
    s.tradeMode = (int)p.trade_mode(); s.tradeCalcMode = (int)p.trade_calc_mode(); s.tradeExeMode = (int)p.trade_exe_mode();
    s.swapMode = (int)p.swap_mode(); s.swapLong = p.swap_long(); s.swapShort = p.swap_short();
    for (int i = 0; i < p.filling_mode_size(); ++i) s.fillingModes.push_back((int)p.filling_mode(i));
    s.visible = p.visible(); s.selected = p.select(); s.exist = p.exist(); s.time = p.time().seconds();
    return s;
}

Position positionFrom(const pb::PositionInfo& p) {
    Position o; o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.type(); o.volume = p.volume();
    o.priceOpen = p.price_open(); o.priceCurrent = p.price_current(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit();
    o.profit = p.profit(); o.swap = p.swap(); o.commission = p.position_commission(); o.magic = p.magic_number();
    o.comment = p.comment(); o.openTime = p.open_time().seconds(); o.identifier = p.identifier(); o.reason = (int)p.reason();
    return o;
}

PendingOrder pendingFrom(const pb::OpenedOrderInfo& p) {
    PendingOrder o; o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.type(); o.state = (int)p.state();
    o.volumeInitial = p.volume_initial(); o.volumeCurrent = p.volume_current(); o.priceOpen = p.price_open();
    o.priceCurrent = p.price_current(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit(); o.stopLimit = p.stop_limit();
    o.magic = p.magic_number(); o.comment = p.comment(); o.timeSetup = p.time_setup().seconds();
    o.timeExpiration = p.time_expiration().seconds(); o.positionId = p.position_id();
    return o;
}

Deal dealFrom(const pb::DealHistoryData& d) {
    Deal o; o.ticket = (int64_t)d.ticket(); o.symbol = d.symbol(); o.type = (int)d.type(); o.entry = (int)d.entry_type();
    o.volume = d.volume(); o.price = d.price(); o.profit = d.profit(); o.commission = d.commission(); o.swap = d.swap();
    o.fee = d.fee(); o.stopLoss = d.stop_loss(); o.takeProfit = d.take_profit(); o.positionId = (int64_t)d.position_id();
    o.time = d.time().seconds(); o.comment = d.comment(); o.reason = (int)d.reason();
    return o;
}

HistoryOrder historyOrderFrom(const pb::OrderHistoryData& h) {
    HistoryOrder o; o.ticket = (int64_t)h.ticket(); o.symbol = h.symbol(); o.type = (int)h.type(); o.state = (int)h.state();
    o.priceOpen = h.price_open(); o.priceCurrent = h.price_current(); o.stopLoss = h.stop_loss(); o.takeProfit = h.take_profit();
    o.volumeInitial = h.volume_initial(); o.volumeCurrent = h.volume_current(); o.magic = h.magic_number(); o.comment = h.comment();
    o.setupTime = h.setup_time().seconds(); o.doneTime = h.done_time().seconds(); o.positionId = (int64_t)h.position_id();
    return o;
}

// OrderSend/OrderModify share the same data shape; OrderClose is handled inline (different fields).
template <class Data>
OrderResult orderResultFrom(const Data& d) {
    OrderResult r; r.retcode = d.returned_code(); r.deal = (int64_t)d.deal(); r.order = (int64_t)d.order(); r.ticket = r.order;
    r.volume = d.volume(); r.price = d.price(); r.bid = d.bid(); r.ask = d.ask(); r.comment = d.comment();
    r.retcodeName = d.returned_string_code(); r.retcodeDescription = d.returned_code_description(); r.requestId = d.request_id();
    return r;
}

} // namespace
#pragma endregion

// ======================================================================================
// MT5Service::Impl — owns the MT5Account and the UTC policy
// ======================================================================================
#pragma region Impl
struct MT5Service::Impl {
    MT5Account account;
    bool utcTimes{true};
    std::string server;   // remembered from connect() so accountSummary() can report it

    explicit Impl(const std::string& host, int port, const std::string& apiKey, const std::string& name)
        : account(host, port, apiKey, name) {}

    // Broker-clock offset in seconds (cached inside MT5Account); 0 if we are not converting.
    int64_t off() { return account.serverOffsetSeconds(); }
    // UTC -> broker server time (for request windows); a 0 (unset) or non-UTC mode is passed through.
    int64_t in(int64_t utc)   { return (utc != 0 && utcTimes) ? utc + off() : utc; }
    // Broker server time -> UTC (for reply times).
    int64_t out(int64_t srv)  { return (srv != 0 && utcTimes) ? srv - off() : srv; }
    // Same as out() but for millisecond timestamps.
    int64_t outMs(int64_t ms) { return (ms != 0 && utcTimes) ? ms - off() * 1000 : ms; }
};
#pragma endregion

// ======================================================================================
// MT5Service — lifecycle / connection / time
// ======================================================================================
#pragma region Lifecycle / connection / time
MT5Service::MT5Service(const std::string& host, int port, const std::string& apiKey,
                       const std::string& name, bool utcTimes)
    : m_impl(new Impl(host, port, apiKey, name)) {
    m_impl->utcTimes = utcTimes;
}
MT5Service::~MT5Service() = default;

MT5Account& MT5Service::account() { return m_impl->account; }

// Remember the server name (for accountSummary) and delegate the actual ConnectEx to Layer 1.
void MT5Service::connect(int64_t login, const std::string& password, const std::string& server,
                         const std::string& name, int timeoutSeconds) {
    m_impl->server = server;
    m_impl->account.connect(login, password, server, name, timeoutSeconds);
}
void MT5Service::reconnect() { m_impl->account.reconnect(); }
bool MT5Service::isConnected() const { return m_impl->account.isConnected(); }
std::string MT5Service::terminalGuid() const { return m_impl->account.terminalGuid(); }
void MT5Service::disconnect(const std::string& reason) { m_impl->account.disconnect(reason); }
void MT5Service::close() { m_impl->account.close(); }
ClientOptions& MT5Service::options() { return m_impl->account.options(); }

int64_t MT5Service::serverOffsetSeconds() { return m_impl->account.serverOffsetSeconds(); }
int64_t MT5Service::toUtc(int64_t serverTime) { return serverTime ? serverTime - m_impl->off() : 0; }
int64_t MT5Service::toServerTime(int64_t utc) { return utc ? utc + m_impl->off() : 0; }
#pragma endregion

// ======================================================================================
// MT5Service — account
// ======================================================================================
#pragma region Account
// Fetch the raw AccountSummary and project it onto AccountInfo, filling both the raw server
// time and the same instant in UTC (from the broker's reported shift).
AccountInfo MT5Service::accountSummary() {
    // Step 1: one low-level AccountSummary call (also caches the server offset in Layer 1).
    pb::AccountSummaryData d = m_impl->account.accountSummary();
    // Step 2: copy the scalar fields across.
    AccountInfo a;
    a.login = d.account_login(); a.currency = d.account_currency(); a.balance = d.account_balance();
    a.equity = d.account_equity(); a.credit = d.account_credit(); a.leverage = d.account_leverage();
    a.name = d.account_user_name(); a.company = d.account_company_name();
    a.tradeMode = (TradeMode)(int)d.account_trade_mode();
    // Step 3: derive the UTC instant from the broker server time and its reported UTC shift.
    a.serverTime = d.server_time().seconds(); a.utcShiftMinutes = d.utc_timezone_server_time_shift_minutes();
    a.serverTimeUtc = a.serverTime ? a.serverTime - a.utcShiftMinutes * 60 : 0;
    a.server = m_impl->server;
    return a;
}

// One numeric account property. Build the typed request, call Layer 1, unwrap requested_value.
double MT5Service::accountInfoDouble(int propertyId) {
    pb::AccountInfoDoubleRequest req; req.set_property_id((pb::AccountInfoDoublePropertyType)propertyId);
    return m_impl->account.accountInfoDouble(req).requested_value();
}
int64_t MT5Service::accountInfoInteger(int propertyId) {
    pb::AccountInfoIntegerRequest req; req.set_property_id((pb::AccountInfoIntegerPropertyType)propertyId);
    return m_impl->account.accountInfoInteger(req).requested_value();
}
std::string MT5Service::accountInfoString(int propertyId) {
    pb::AccountInfoStringRequest req; req.set_property_id((pb::AccountInfoStringPropertyType)propertyId);
    return m_impl->account.accountInfoString(req).requested_value();
}
#pragma endregion

// ======================================================================================
// MT5Service — symbols / quotes
// ======================================================================================
#pragma region Symbols / quotes
bool MT5Service::symbolSelect(const std::string& symbol, bool select) {
    pb::SymbolSelectRequest req; req.set_symbol(symbol); req.set_select(select);
    return m_impl->account.symbolSelect(req).success();
}
bool MT5Service::symbolExist(const std::string& symbol) {
    pb::SymbolExistRequest req; req.set_name(symbol);
    return m_impl->account.symbolExist(req).exists();
}
bool MT5Service::symbolIsSynchronized(const std::string& symbol) {
    pb::SymbolIsSynchronizedRequest req; req.set_symbol(symbol);
    return m_impl->account.symbolIsSynchronized(req).synchronized();
}
std::vector<std::string> MT5Service::symbolList() {
    pb::SymbolListData d = m_impl->account.symbolList();
    std::vector<std::string> out;
    out.reserve(d.symbols_size());
    for (int i = 0; i < d.symbols_size(); ++i) out.push_back(d.symbols(i));
    return out;
}

// Latest tick, converted to a friendly Tick with its times shifted to UTC (when utcTimes).
Tick MT5Service::symbolInfoTick(const std::string& symbol) {
    // Step 1: raw tick from Layer 1 (times are broker server time).
    pb::SymbolInfoTickRequest req; req.set_symbol(symbol);
    pb::MrpcMqlTick raw = m_impl->account.symbolInfoTick(req);
    // Step 2: convert + normalise the time fields to UTC.
    Tick t = tickFrom(raw, symbol);
    t.time = m_impl->out(t.time);
    t.timeMsc = m_impl->outMs(t.timeMsc);
    return t;
}

// All params in one call; convert each entry and shift its quote time to UTC.
std::vector<SymbolInfo> MT5Service::symbolParamsMany(const std::string& nameFilter) {
    pb::SymbolParamsManyRequest req;
    if (!nameFilter.empty()) req.set_symbol_name(nameFilter);
    pb::SymbolParamsManyData d = m_impl->account.symbolParamsMany(req);
    std::vector<SymbolInfo> out;
    out.reserve(d.symbol_infos_size());
    for (int i = 0; i < d.symbol_infos_size(); ++i) {
        SymbolInfo s = symbolFrom(d.symbol_infos(i));
        s.time = m_impl->out(s.time);
        out.push_back(s);
    }
    return out;
}

// Exact-name lookup built on symbolParamsMany (the filter may return partial matches).
SymbolInfo MT5Service::symbolParams(const std::string& symbol) {
    auto all = symbolParamsMany(symbol);
    for (const auto& s : all) if (s.name == symbol) return s;
    throw ApiError("SYMBOL_NOT_FOUND", "SymbolParamsMany returned no entry named '" + symbol + "' (" +
                   std::to_string(all.size()) + " partial matches)", 0, 0, "", "SymbolParamsMany");
}

double MT5Service::symbolInfoDouble(const std::string& symbol, int propertyId) {
    pb::SymbolInfoDoubleRequest req; req.set_symbol(symbol); req.set_type((pb::SymbolInfoDoubleProperty)propertyId);
    return m_impl->account.symbolInfoDouble(req).value();
}
int64_t MT5Service::symbolInfoInteger(const std::string& symbol, int propertyId) {
    pb::SymbolInfoIntegerRequest req; req.set_symbol(symbol); req.set_type((pb::SymbolInfoIntegerProperty)propertyId);
    return m_impl->account.symbolInfoInteger(req).value();
}
std::string MT5Service::symbolInfoString(const std::string& symbol, int propertyId) {
    pb::SymbolInfoStringRequest req; req.set_symbol(symbol); req.set_type((pb::SymbolInfoStringProperty)propertyId);
    return m_impl->account.symbolInfoString(req).value();
}

// Count of symbols (all or Market Watch selection).
int MT5Service::symbolsTotal(bool selectedOnly) {
    pb::SymbolsTotalRequest req; req.set_mode(selectedOnly);
    return m_impl->account.symbolsTotal(req).total();
}

// Symbol name by index.
std::string MT5Service::symbolName(int index, bool selectedOnly) {
    pb::SymbolNameRequest req; req.set_index(index); req.set_selected(selectedOnly);
    return m_impl->account.symbolName(req).name();
}

// Initial/maintenance margin rates for a symbol + order type.
MarginRate MT5Service::symbolInfoMarginRate(const std::string& symbol, int orderType) {
    pb::SymbolInfoMarginRateRequest req; req.set_symbol(symbol); req.set_order_type((pb::ENUM_ORDER_TYPE)orderType);
    pb::SymbolInfoMarginRateData d = m_impl->account.symbolInfoMarginRate(req);
    return MarginRate{ d.initial_margin_rate(), d.maintenance_margin_rate() };
}

// Quote-session window; from/to are broker server time -> shifted to UTC.
SessionTime MT5Service::symbolInfoSessionQuote(const std::string& symbol, int dayOfWeek, uint32_t sessionIndex) {
    pb::SymbolInfoSessionQuoteRequest req;
    req.set_symbol(symbol); req.set_day_of_week((pb::DayOfWeek)dayOfWeek); req.set_session_index(sessionIndex);
    pb::SymbolInfoSessionQuoteData d = m_impl->account.symbolInfoSessionQuote(req);
    return SessionTime{ m_impl->out(d.from().seconds()), m_impl->out(d.to().seconds()) };
}

// Trade-session window; from/to shifted to UTC.
SessionTime MT5Service::symbolInfoSessionTrade(const std::string& symbol, int dayOfWeek, uint32_t sessionIndex) {
    pb::SymbolInfoSessionTradeRequest req;
    req.set_symbol(symbol); req.set_day_of_week((pb::DayOfWeek)dayOfWeek); req.set_session_index(sessionIndex);
    pb::SymbolInfoSessionTradeData d = m_impl->account.symbolInfoSessionTrade(req);
    return SessionTime{ m_impl->out(d.from().seconds()), m_impl->out(d.to().seconds()) };
}

// Tick value + sizing for several symbols at once.
std::vector<TickValueSize> MT5Service::tickValueWithSize(const std::vector<std::string>& symbols) {
    pb::TickValueWithSizeRequest req;
    for (const auto& s : symbols) req.add_symbol_names(s);
    pb::TickValueWithSizeData d = m_impl->account.tickValueWithSize(req);
    std::vector<TickValueSize> out;
    out.reserve(d.symbol_tick_size_infos_size());
    for (int i = 0; i < d.symbol_tick_size_infos_size(); ++i) {
        const auto& t = d.symbol_tick_size_infos(i);
        out.push_back(TickValueSize{ t.name(), t.tradetickvalue(), t.tradetickvalueprofit(),
                                     t.tradetickvalueloss(), t.tradeticksize(), t.tradecontractsize() });
    }
    return out;
}
#pragma endregion

// ======================================================================================
// MT5Service — market depth (DOM)
// ======================================================================================
#pragma region Market depth (DOM)
// Subscribe the terminal to a symbol's order book.
bool MT5Service::marketBookAdd(const std::string& symbol) {
    pb::MarketBookAddRequest req; req.set_symbol(symbol);
    return m_impl->account.marketBookAdd(req).opened_successfully();
}

// Current order-book snapshot (price levels).
std::vector<BookEntry> MT5Service::marketBookGet(const std::string& symbol) {
    pb::MarketBookGetRequest req; req.set_symbol(symbol);
    pb::MarketBookGetData d = m_impl->account.marketBookGet(req);
    std::vector<BookEntry> out;
    out.reserve(d.mql_book_infos_size());
    for (int i = 0; i < d.mql_book_infos_size(); ++i) {
        const auto& b = d.mql_book_infos(i);
        out.push_back(BookEntry{ (int)b.type(), b.price(), (int64_t)b.volume(), b.volume_real() });
    }
    return out;
}

// Unsubscribe the terminal from a symbol's order book.
bool MT5Service::marketBookRelease(const std::string& symbol) {
    pb::MarketBookReleaseRequest req; req.set_symbol(symbol);
    return m_impl->account.marketBookRelease(req).closed_successfully();
}
#pragma endregion

// ======================================================================================
// MT5Service — bars
// ======================================================================================
#pragma region Bars
// Build the request in broker server time (in()), fetch bars, convert each back to UTC (out())
// and sort oldest-first. A zero from/to with max_bars means "the newest N bars".
std::vector<Bar> MT5Service::priceHistory(const std::string& symbol, Timeframe tf, int64_t fromUnix,
                                          int64_t toUnix, int maxBars) {
    // Step 1: assemble the request (times go out as broker server time).
    pb::PriceHistoryRequest req;
    req.set_symbol(symbol);
    req.set_timeframe((int32_t)tf);
    req.set_from_time(m_impl->in(fromUnix));
    req.set_to_time(m_impl->in(toUnix));
    if (maxBars > 0) req.set_max_bars(maxBars);
    // Step 2: call Layer 1.
    pb::PriceHistoryData d = m_impl->account.priceHistory(req);
    // Step 3: convert bars and shift their open time back to UTC.
    std::vector<Bar> out;
    out.reserve(d.bars_size());
    for (int i = 0; i < d.bars_size(); ++i) {
        const auto& b = d.bars(i);
        Bar o; o.time = m_impl->out(b.time()); o.open = b.open(); o.high = b.high(); o.low = b.low(); o.close = b.close();
        o.tickVolume = b.tick_volume(); o.realVolume = b.real_volume(); o.spread = b.spread();
        out.push_back(o);
    }
    // Step 4: the wire order is not guaranteed — present bars oldest-first.
    std::sort(out.begin(), out.end(), [](const Bar& a, const Bar& b) { return a.time < b.time; });
    return out;
}

// The newest `count` bars: ask for them with a zero window + max_bars, then trim any overshoot.
std::vector<Bar> MT5Service::lastBars(const std::string& symbol, Timeframe tf, int count) {
    auto bars = priceHistory(symbol, tf, 0, 0, count);
    if ((int)bars.size() > count) bars.erase(bars.begin(), bars.end() - count);
    return bars;
}
#pragma endregion

// ======================================================================================
// MT5Service — trading
// ======================================================================================
#pragma region Trading
// Translate the friendly OrderRequest into an OrderSendRequest, send it, and project the result.
OrderResult MT5Service::orderSend(const OrderRequest& q) {
    // Step 1: map fields; only set the optional ones when meaningful so defaults stay unset.
    pb::OrderSendRequest req;
    req.set_symbol(q.symbol);
    req.set_operation((pb::TMT5_ENUM_ORDER_TYPE)(int)q.action);
    req.set_volume(q.volume);
    if (q.price > 0) req.set_price(q.price);
    req.set_slippage((uint64_t)std::max(0, q.slippage));
    if (q.stopLoss > 0) req.set_stop_loss(q.stopLoss);
    if (q.takeProfit > 0) req.set_take_profit(q.takeProfit);
    if (q.stopLimit > 0) req.set_stop_limit_price(q.stopLimit);
    if (!q.comment.empty()) req.set_comment(q.comment);
    if (q.magic != 0) req.set_expert_id((uint64_t)q.magic);
    if (q.expiration > 0) {
        // Step 1a: an expiration turns this into a time-limited pending order (server time on wire).
        req.set_expiration_time_type(pb::TMT5_ORDER_TIME_SPECIFIED);
        req.mutable_expiration_time()->set_seconds(m_impl->in(q.expiration));
    }
    // Step 2: send via Layer 1 and project onto OrderResult.
    return orderResultFrom(m_impl->account.orderSend(req));
}

// Modify SL/TP (and price/stop-limit for pendings) of an existing order/position.
OrderResult MT5Service::orderModify(int64_t ticket, double stopLoss, double takeProfit,
                                    double price, double stopLimit) {
    pb::OrderModifyRequest req;
    req.set_ticket((uint64_t)ticket);
    req.set_stop_loss(stopLoss);
    req.set_take_profit(takeProfit);
    if (price > 0) req.set_price(price);
    if (stopLimit > 0) req.set_stop_limit(stopLimit);
    return orderResultFrom(m_impl->account.orderModify(req));
}

// Close a position / delete a pending order. If no volume is given, look up the full volume
// from the live snapshot so a bare ticket closes the whole position.
OrderResult MT5Service::orderClose(int64_t ticket, double volume, int slippage) {
    // Step 1: resolve volume = 0 to the position's / order's current volume.
    if (volume <= 0) {
        auto open = openedOrders();
        for (const auto& p : open.positions) if (p.ticket == ticket) { volume = p.volume; break; }
        if (volume <= 0)
            for (const auto& o : open.orders) if (o.ticket == ticket) { volume = o.volumeCurrent; break; }
        if (volume <= 0)
            throw ApiError("POSITION_NOT_FOUND", "orderClose: ticket " + std::to_string(ticket) +
                           " is not an open position or pending order", 0, 0, "", "OrderClose");
    }
    // Step 2: issue the close.
    pb::OrderCloseRequest req;
    req.set_ticket((uint64_t)ticket);
    req.set_volume(volume);
    req.set_slippage(slippage);
    pb::OrderCloseData d = m_impl->account.orderClose(req);
    // Step 3: OrderCloseData has a different shape (no deal/price) — fill what it gives us.
    OrderResult r;
    r.retcode = d.returned_code(); r.retcodeName = d.returned_string_code(); r.retcodeDescription = d.returned_code_description();
    r.ticket = ticket; r.order = ticket; r.volume = volume; r.comment = "close_mode=" + std::to_string((int)d.close_mode());
    return r;
}

// Snapshot of positions + pending orders, converted and with all times shifted to UTC.
OpenedOrders MT5Service::openedOrders() {
    // Step 1: ask Layer 1, sorted by open time ascending.
    pb::OpenedOrdersRequest req;
    req.set_inputsortmode(pb::BMT5_OPENED_ORDER_SORT_BY_OPEN_TIME_ASC);
    pb::OpenedOrdersData d = m_impl->account.openedOrders(req);
    // Step 2: convert positions (shift open time to UTC).
    OpenedOrders out;
    for (int i = 0; i < d.position_infos_size(); ++i) {
        Position p = positionFrom(d.position_infos(i));
        p.openTime = m_impl->out(p.openTime);
        out.positions.push_back(p);
    }
    // Step 3: convert pending orders (shift setup/expiration times to UTC).
    for (int i = 0; i < d.opened_orders_size(); ++i) {
        PendingOrder o = pendingFrom(d.opened_orders(i));
        o.timeSetup = m_impl->out(o.timeSetup);
        o.timeExpiration = m_impl->out(o.timeExpiration);
        out.orders.push_back(o);
    }
    return out;
}

// Open positions, optionally filtered by magic number client-side.
std::vector<Position> MT5Service::positions(int64_t magic) {
    auto open = openedOrders();
    if (magic < 0) return open.positions;
    std::vector<Position> out;
    for (const auto& p : open.positions) if (p.magic == magic) out.push_back(p);
    return out;
}

int MT5Service::positionsTotal() {
    return m_impl->account.positionsTotal().total_positions();
}

// Just the tickets of live positions and pending orders.
OpenedTickets MT5Service::openedOrdersTickets() {
    pb::OpenedOrdersTicketsRequest req;
    pb::OpenedOrdersTicketsData d = m_impl->account.openedOrdersTickets(req);
    OpenedTickets out;
    out.positionTickets.reserve(d.opened_position_tickets_size());
    for (int i = 0; i < d.opened_position_tickets_size(); ++i) out.positionTickets.push_back(d.opened_position_tickets(i));
    out.orderTickets.reserve(d.opened_orders_tickets_size());
    for (int i = 0; i < d.opened_orders_tickets_size(); ++i) out.orderTickets.push_back(d.opened_orders_tickets(i));
    return out;
}

// Dry-run validate an order: build an MqlTradeRequest from the friendly OrderRequest and project
// the MqlTradeCheckResult onto OrderCheckResult.
OrderCheckResult MT5Service::orderCheck(const OrderRequest& q) {
    // Step 1: translate OrderRequest into the nested MqlTradeRequest.
    pb::OrderCheckRequest req;
    auto* t = req.mutable_mql_trade_request();
    const bool market = (q.action == TradeAction::Buy || q.action == TradeAction::Sell);
    t->set_action((pb::MRPC_ENUM_TRADE_REQUEST_ACTIONS)(market ? 0 : 1));  // DEAL vs PENDING
    t->set_symbol(q.symbol);
    t->set_volume(q.volume);
    t->set_order_type((pb::ENUM_ORDER_TYPE_TF)(int)q.action);
    if (q.price > 0) t->set_price(q.price);
    if (q.stopLoss > 0) t->set_stop_loss(q.stopLoss);
    if (q.takeProfit > 0) t->set_take_profit(q.takeProfit);
    if (q.magic != 0) t->set_expert_advisor_magic_number((uint64_t)q.magic);
    if (!q.comment.empty()) t->set_comment(q.comment);
    // Step 2: call Layer 1 and project the check result.
    const auto& r = m_impl->account.orderCheck(req).mql_trade_check_result();
    OrderCheckResult out;
    out.retcode = r.returned_code(); out.balance = r.balance_after_deal(); out.equity = r.equity_after_deal();
    out.profit = r.profit(); out.margin = r.margin(); out.freeMargin = r.free_margin();
    out.marginLevel = r.margin_level(); out.comment = r.comment();
    return out;
}

// Margin required to open a given order.
double MT5Service::orderCalcMargin(const std::string& symbol, int orderType, double volume, double price) {
    pb::OrderCalcMarginRequest req;
    req.set_symbol(symbol); req.set_order_type((pb::ENUM_ORDER_TYPE_TF)orderType); req.set_volume(volume); req.set_open_price(price);
    return m_impl->account.orderCalcMargin(req).margin();
}

// Profit a price move would yield.
double MT5Service::orderCalcProfit(const std::string& symbol, int orderType, double volume,
                                   double openPrice, double closePrice) {
    pb::OrderCalcProfitRequest req;
    req.set_symbol(symbol); req.set_order_type((pb::ENUM_ORDER_TYPE_TF)orderType);
    req.set_volume(volume); req.set_open_price(openPrice); req.set_close_price(closePrice);
    return m_impl->account.orderCalcProfit(req).profit();
}
#pragma endregion

// ======================================================================================
// MT5Service — history
// ======================================================================================
#pragma region History
// Closed orders+deals: window goes out in server time, each returned time comes back as UTC.
std::vector<HistoryItem> MT5Service::orderHistory(int64_t fromUnix, int64_t toUnix) {
    // Step 1: assemble the request window (server time) and sort by close time.
    pb::OrderHistoryRequest req;
    req.mutable_inputfrom()->set_seconds(m_impl->in(fromUnix));
    req.mutable_inputto()->set_seconds(m_impl->in(toUnix));
    req.set_inputsortmode(pb::BMT5_SORT_BY_CLOSE_TIME_ASC);
    req.set_pagenumber(0);
    req.set_itemsperpage(0);
    // Step 2: call Layer 1.
    pb::OrdersHistoryData d = m_impl->account.orderHistory(req);
    // Step 3: each entry may carry an order, a deal, or both — convert whatever is present.
    std::vector<HistoryItem> out;
    for (int i = 0; i < d.history_data_size(); ++i) {
        const auto& h = d.history_data(i);
        HistoryItem item;
        if (h.has_history_order()) {
            item.hasOrder = true; item.order = historyOrderFrom(h.history_order());
            item.order.setupTime = m_impl->out(item.order.setupTime);
            item.order.doneTime = m_impl->out(item.order.doneTime);
        }
        if (h.has_history_deal()) {
            item.hasDeal = true; item.deal = dealFrom(h.history_deal());
            item.deal.time = m_impl->out(item.deal.time);
        }
        out.push_back(item);
    }
    return out;
}

// Closed positions in the window, converted with open/close times shifted to UTC.
std::vector<PositionHistory> MT5Service::positionsHistory(int64_t fromUnix, int64_t toUnix) {
    // Step 1: window in server time, sorted by open time.
    pb::PositionsHistoryRequest req;
    req.mutable_position_open_time_from()->set_seconds(m_impl->in(fromUnix));
    req.mutable_position_open_time_to()->set_seconds(m_impl->in(toUnix));
    req.set_sort_type(pb::AH_POSITION_OPEN_TIME_ASC);
    // Step 2: call Layer 1.
    pb::PositionsHistoryData d = m_impl->account.positionsHistory(req);
    // Step 3: convert each closed position.
    std::vector<PositionHistory> out;
    for (int i = 0; i < d.history_positions_size(); ++i) {
        const auto& p = d.history_positions(i);
        PositionHistory o;
        o.ticket = (int64_t)p.position_ticket(); o.symbol = p.symbol(); o.orderType = (int)p.order_type(); o.volume = p.volume();
        o.openPrice = p.open_price(); o.closePrice = p.close_price(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit();
        o.profit = p.profit(); o.swap = p.swap(); o.commission = p.commission(); o.fee = p.fee();
        o.openTime = m_impl->out(p.open_time().seconds()); o.closeTime = m_impl->out(p.close_time().seconds());
        o.magic = p.magic(); o.comment = p.comment();
        out.push_back(o);
    }
    return out;
}
#pragma endregion

// ======================================================================================
// MT5Service — streaming
// ======================================================================================
#pragma region Streaming converters
namespace {
// Shift a protobuf Timestamp's seconds to UTC (sh == broker offset, or 0 when not converting).
inline int64_t sh(const ::google::protobuf::Timestamp& ts, int64_t s) {
    int64_t v = ts.seconds();
    return (v && s) ? v - s : v;
}

EventAccountInfo eventAccountFrom(const pb::OnEventAccountInfo& a) {
    EventAccountInfo o;
    o.login = a.login(); o.balance = a.balance(); o.credit = a.credit(); o.equity = a.equity();
    o.margin = a.margin(); o.freeMargin = a.free_margin(); o.marginLevel = a.margin_level(); o.profit = a.profit();
    return o;
}

TradeOrderInfo tradeOrderFrom(const pb::OnTradeOrderInfo& p, int64_t s) {
    TradeOrderInfo o;
    o.index = p.index(); o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.order_type();
    o.state = (int)p.state(); o.typeFilling = (int)p.order_type_filling(); o.timeType = (int)p.time_type();
    o.reason = (int)p.order_reason(); o.volumeInitial = p.volume_initial(); o.volumeCurrent = p.volume_current();
    o.priceOpen = p.price_open(); o.priceCurrent = p.price_current(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit();
    o.stopLimit = p.stop_limit(); o.magic = p.magic(); o.positionId = (int64_t)p.position_id(); o.positionById = (int64_t)p.position_by_id();
    o.comment = p.comment(); o.setupTime = sh(p.setup_time(), s); o.expiration = sh(p.time_expiration(), s);
    return o;
}

TradePositionInfo tradePositionFrom(const pb::OnTradePositionInfo& p, int64_t s) {
    TradePositionInfo o;
    o.index = p.index(); o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.type(); o.reason = (int)p.reason();
    o.volume = p.volume(); o.priceOpen = p.price_open(); o.priceCurrent = p.price_current(); o.stopLoss = p.sl(); o.takeProfit = p.tp();
    o.profit = p.profit(); o.swap = p.swap(); o.magic = p.magic(); o.fromPendingOrder = p.from_pending_order(); o.comment = p.comment();
    o.positionTime = sh(p.position_time(), s); o.lastUpdateTime = sh(p.last_update_time(), s);
    return o;
}

TradeHistoryOrderInfo tradeHistoryOrderFrom(const pb::OnTradeHistoryOrderInfo& p, int64_t s) {
    TradeHistoryOrderInfo o;
    o.index = p.index(); o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.order_type();
    o.state = (int)p.state(); o.typeFilling = (int)p.order_type_filling(); o.timeType = (int)p.type_time();
    o.reason = (int)p.reason(); o.volumeInitial = p.volume_initial(); o.volumeCurrent = p.volume_current();
    o.priceOpen = p.price_open(); o.priceCurrent = p.price_current(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit();
    o.stopLimit = p.stop_limit(); o.magic = p.magic(); o.positionId = (int64_t)p.position_id(); o.positionBy = (int64_t)p.position_by();
    o.comment = p.comment(); o.setupTime = sh(p.setup_time(), s); o.doneTime = sh(p.done_time(), s); o.expiration = sh(p.time_expiration(), s);
    return o;
}

TradeHistoryDealInfo tradeHistoryDealFrom(const pb::OnTradeHistoryDealInfo& p, int64_t s) {
    TradeHistoryDealInfo o;
    o.index = p.index(); o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.type(); o.entry = (int)p.entry();
    o.reason = (int)p.reason(); o.volume = p.volume(); o.price = p.price(); o.profit = p.profit(); o.commission = p.commission();
    o.swap = p.swap(); o.fee = p.fee(); o.stopLoss = p.sl(); o.takeProfit = p.tp(); o.magic = p.magic();
    o.orderTicket = (int64_t)p.order_ticket(); o.positionId = (int64_t)p.deal_position_id(); o.comment = p.comment();
    o.time = sh(p.deal_time(), s);
    return o;
}

TradeTransaction tradeTransactionFrom(const pb::MqlTradeTransaction& t, int64_t s) {
    TradeTransaction o;
    o.type = (int)t.type(); o.dealTicket = (int64_t)t.deal_ticket(); o.orderTicket = (int64_t)t.order_ticket();
    o.positionTicket = (int64_t)t.position_ticket(); o.positionByOpposite = (int64_t)t.position_by_opposite_position();
    o.symbol = t.symbol(); o.dealType = (int)t.deal_type(); o.orderType = (int)t.order_type(); o.orderState = (int)t.order_state();
    o.orderTimeType = (int)t.order_time_type(); o.price = t.price(); o.priceTriggerStopLimit = t.price_trigger_stop_limit();
    o.priceStopLoss = t.price_stop_loss(); o.priceTakeProfit = t.price_take_profit(); o.volume = t.volume();
    o.orderExpiration = sh(t.order_expiration_time(), s);
    return o;
}

TradeRequestInfo tradeRequestFrom(const pb::MqlTradeRequest& t, int64_t s) {
    TradeRequestInfo o;
    o.operationType = (int)t.trade_operation_type(); o.orderType = (int)t.order_type(); o.typeFilling = (int)t.order_type_filling();
    o.timeType = (int)t.type_time(); o.magic = (int64_t)t.magic(); o.orderTicket = (int64_t)t.order_ticket();
    o.positionTicket = (int64_t)t.position_ticket(); o.positionByOpposite = (int64_t)t.position_by_opposite_position();
    o.symbol = t.symbol(); o.volumeLots = t.requested_deal_volume_lots(); o.price = t.price(); o.stopLoss = t.stop_loss();
    o.takeProfit = t.take_profit(); o.stopLimit = t.stopl_imit(); o.deviation = t.deviation(); o.comment = t.order_comment();
    o.orderExpiration = sh(t.order_expiration_time(), s);
    return o;
}

TradeResultInfo tradeResultFrom(const pb::MqlTradeResult& t) {
    TradeResultInfo o;
    o.retcode = t.trade_return_code(); o.retcodeInt = t.trade_return_int_code(); o.dealTicket = (int64_t)t.deal_ticket();
    o.orderTicket = (int64_t)t.order_ticket(); o.dealVolume = t.deal_volume(); o.dealPrice = t.deal_price();
    o.bid = t.current_bid(); o.ask = t.current_ask(); o.comment = t.broker_comment_to_operation();
    o.requestId = t.terminal_dispatch_request_id(); o.returnCodeExternal = t.return_code_external();
    return o;
}

PositionProfitInfo positionProfitFrom(const pb::OnPositionProfitPositionInfo& p) {
    PositionProfitInfo o;
    o.index = p.index(); o.ticket = (int64_t)p.ticket(); o.symbol = p.position_symbol(); o.profit = p.profit();
    return o;
}

// --- top-level event assemblers ---

TradeEvent tradeEventFrom(const pb::OnTradeData& d, int64_t s) {
    TradeEvent e;
    e.type = (int)d.type(); e.terminalGuid = d.terminal_instance_guid_id();
    if (d.has_account_info()) e.account = eventAccountFrom(d.account_info());
    if (d.has_event_data()) {
        const auto& ev = d.event_data();
        for (int i = 0; i < ev.new_orders_size(); ++i) e.newOrders.push_back(tradeOrderFrom(ev.new_orders(i), s));
        for (int i = 0; i < ev.disappeared_orders_size(); ++i) e.disappearedOrders.push_back(tradeOrderFrom(ev.disappeared_orders(i), s));
        for (int i = 0; i < ev.state_changed_orders_size(); ++i)
            e.stateChangedOrders.push_back({ tradeOrderFrom(ev.state_changed_orders(i).previous_order(), s),
                                             tradeOrderFrom(ev.state_changed_orders(i).current_order(), s) });
        for (int i = 0; i < ev.new_history_orders_size(); ++i) e.newHistoryOrders.push_back(tradeHistoryOrderFrom(ev.new_history_orders(i), s));
        for (int i = 0; i < ev.disappeared_history_orders_size(); ++i) e.disappearedHistoryOrders.push_back(tradeHistoryOrderFrom(ev.disappeared_history_orders(i), s));
        for (int i = 0; i < ev.updated_history_orders_size(); ++i)
            e.updatedHistoryOrders.push_back({ tradeHistoryOrderFrom(ev.updated_history_orders(i).previous_history_order(), s),
                                               tradeHistoryOrderFrom(ev.updated_history_orders(i).current_history_order(), s) });
        for (int i = 0; i < ev.new_history_deals_size(); ++i) e.newHistoryDeals.push_back(tradeHistoryDealFrom(ev.new_history_deals(i), s));
        for (int i = 0; i < ev.disappeared_history_deals_size(); ++i) e.disappearedHistoryDeals.push_back(tradeHistoryDealFrom(ev.disappeared_history_deals(i), s));
        for (int i = 0; i < ev.updated_history_deals_size(); ++i)
            e.updatedHistoryDeals.push_back({ tradeHistoryDealFrom(ev.updated_history_deals(i).previous_history_deal(), s),
                                              tradeHistoryDealFrom(ev.updated_history_deals(i).current_history_deal(), s) });
        for (int i = 0; i < ev.new_positions_size(); ++i) e.newPositions.push_back(tradePositionFrom(ev.new_positions(i), s));
        for (int i = 0; i < ev.disappeared_positions_size(); ++i) e.disappearedPositions.push_back(tradePositionFrom(ev.disappeared_positions(i), s));
        for (int i = 0; i < ev.updated_positions_size(); ++i)
            e.updatedPositions.push_back({ tradePositionFrom(ev.updated_positions(i).previous_position(), s),
                                           tradePositionFrom(ev.updated_positions(i).current_position(), s) });
    }
    return e;
}

TransactionEvent transactionEventFrom(const pb::OnTradeTransactionData& d, int64_t s) {
    TransactionEvent e;
    e.type = (int)d.type(); e.terminalGuid = d.terminal_instance_guid_id();
    if (d.has_account_info()) e.account = eventAccountFrom(d.account_info());
    if (d.has_trade_transaction()) e.transaction = tradeTransactionFrom(d.trade_transaction(), s);
    if (d.has_trade_request()) e.request = tradeRequestFrom(d.trade_request(), s);
    if (d.has_trade_result()) e.result = tradeResultFrom(d.trade_result());
    return e;
}

PositionProfitEvent positionProfitEventFrom(const pb::OnPositionProfitData& d) {
    PositionProfitEvent e;
    e.type = (int)d.type(); e.terminalGuid = d.terminal_instance_guid_id();
    if (d.has_account_info()) e.account = eventAccountFrom(d.account_info());
    for (int i = 0; i < d.new_positions_size(); ++i) e.newPositions.push_back(positionProfitFrom(d.new_positions(i)));
    for (int i = 0; i < d.updated_positions_size(); ++i) e.updatedPositions.push_back(positionProfitFrom(d.updated_positions(i)));
    for (int i = 0; i < d.deleted_positions_size(); ++i) e.deletedPositions.push_back(positionProfitFrom(d.deleted_positions(i)));
    return e;
}

TicketsSnapshot ticketsSnapshotFrom(const pb::OnPositionsAndPendingOrdersTicketsData& d, int64_t s) {
    TicketsSnapshot e;
    e.terminalGuid = d.terminal_instance_guid_id();
    for (int i = 0; i < d.position_tickets_size(); ++i) e.positionTickets.push_back(d.position_tickets(i));
    for (int i = 0; i < d.pending_order_tickets_size(); ++i) e.pendingOrderTickets.push_back(d.pending_order_tickets(i));
    e.serverTime = sh(d.server_time(), s);
    return e;
}
} // namespace
#pragma endregion

#pragma region Streaming
// ServiceStream just owns the low-level AccountStream and forwards the control/query calls; the
// pb -> friendly conversion happens in the callback wrapper installed by each subscribe method.
struct ServiceStream::Impl {
    std::unique_ptr<AccountStream> inner;
};

ServiceStream::ServiceStream() : m_impl(new Impl()) {}
ServiceStream::~ServiceStream() = default;
void ServiceStream::stop() { if (m_impl->inner) m_impl->inner->stop(); }
bool ServiceStream::running() const { return m_impl->inner && m_impl->inner->running(); }
uint64_t ServiceStream::count() const { return m_impl->inner ? m_impl->inner->count() : 0; }
std::string ServiceStream::lastError() const { return m_impl->inner ? m_impl->inner->lastError() : ""; }

// Live ticks: convert each raw payload into a friendly Tick (times shifted to UTC).
std::unique_ptr<ServiceStream> MT5Service::subscribeTicks(
    const std::vector<std::string>& symbols, std::function<void(const Tick&)> callback) {
    const int64_t shift = m_impl->utcTimes ? m_impl->off() : 0;
    pb::OnSymbolTickRequest req;
    for (const auto& s : symbols) req.add_symbol_names(s);
    std::unique_ptr<ServiceStream> s(new ServiceStream());
    auto cb = std::move(callback);
    s->m_impl->inner = m_impl->account.onSymbolTick(req, [cb, shift](const pb::OnSymbolTickData& data) {
        if (!data.has_symbol_tick()) return;
        Tick t = tickFrom(data.symbol_tick());
        if (shift && t.time) t.time -= shift;
        if (shift && t.timeMsc) t.timeMsc -= shift * 1000;
        if (cb) cb(t);
    });
    return s;
}

// Trade events: full conversion of the OnTrade payload into a TradeEvent.
std::unique_ptr<ServiceStream> MT5Service::streamTradeUpdates(std::function<void(const TradeEvent&)> callback) {
    const int64_t shift = m_impl->utcTimes ? m_impl->off() : 0;
    std::unique_ptr<ServiceStream> s(new ServiceStream());
    auto cb = std::move(callback);
    s->m_impl->inner = m_impl->account.onTrade(pb::OnTradeRequest{},
        [cb, shift](const pb::OnTradeData& d) { if (cb) cb(tradeEventFrom(d, shift)); });
    return s;
}

// Trade-transaction log: full conversion into a TransactionEvent.
std::unique_ptr<ServiceStream> MT5Service::streamTransactions(std::function<void(const TransactionEvent&)> callback) {
    const int64_t shift = m_impl->utcTimes ? m_impl->off() : 0;
    std::unique_ptr<ServiceStream> s(new ServiceStream());
    auto cb = std::move(callback);
    s->m_impl->inner = m_impl->account.onTradeTransaction(pb::OnTradeTransactionRequest{},
        [cb, shift](const pb::OnTradeTransactionData& d) { if (cb) cb(transactionEventFrom(d, shift)); });
    return s;
}

// Position-profit snapshots: full conversion into a PositionProfitEvent.
std::unique_ptr<ServiceStream> MT5Service::streamPositionProfits(std::function<void(const PositionProfitEvent&)> callback) {
    std::unique_ptr<ServiceStream> s(new ServiceStream());
    auto cb = std::move(callback);
    s->m_impl->inner = m_impl->account.onPositionProfit(pb::OnPositionProfitRequest{},
        [cb](const pb::OnPositionProfitData& d) { if (cb) cb(positionProfitEventFrom(d)); });
    return s;
}

// Ticket snapshots: full conversion into a TicketsSnapshot.
std::unique_ptr<ServiceStream> MT5Service::streamOpenedTickets(std::function<void(const TicketsSnapshot&)> callback) {
    const int64_t shift = m_impl->utcTimes ? m_impl->off() : 0;
    std::unique_ptr<ServiceStream> s(new ServiceStream());
    auto cb = std::move(callback);
    s->m_impl->inner = m_impl->account.onPositionsAndPendingOrdersTickets(pb::OnPositionsAndPendingOrdersTicketsRequest{},
        [cb, shift](const pb::OnPositionsAndPendingOrdersTicketsData& d) { if (cb) cb(ticketsSnapshotFrom(d, shift)); });
    return s;
}
#pragma endregion

} // namespace metarpc
