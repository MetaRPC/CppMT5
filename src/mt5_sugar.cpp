// MetaRPC CppMT5 — Layer 3 (MT5Sugar) implementation: thin conveniences over MT5Service.
#include "metarpc/mt5_sugar.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <thread>

namespace metarpc {

namespace {
// AccountInfoDoublePropertyType ids (kept local so the sugar header stays protobuf-free).
enum { ACC_CREDIT = 1, ACC_PROFIT = 2, ACC_EQUITY = 3, ACC_MARGIN = 4, ACC_MARGIN_FREE = 5, ACC_MARGIN_LEVEL = 6 };
enum { ACC_BALANCE = 0 };
// ENUM_ORDER_TYPE_TF: market buy = 0 (used for margin calcs).
enum { ORDER_TF_BUY = 0 };

int64_t nowUnix() {
    return (int64_t)std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

// Start of today's UTC day (00:00:00) in unix seconds.
int64_t todayStartUtc() {
    int64_t now = nowUnix();
    return now - (now % 86400);
}
} // namespace

// ======================================================================================
// MT5Sugar::Impl — owns the service; small shared helpers
// ======================================================================================
struct MT5Sugar::Impl {
    MT5Service service;
    explicit Impl(const std::string& host, int port, const std::string& apiKey,
                  const std::string& name, bool utcTimes)
        : service(host, port, apiKey, name, utcTimes) {}

    // One pip for a symbol: a 5-/3-digit quote uses 10 points per pip, others one point.
    double pipSize(const SymbolInfo& s) const {
        return (s.digits == 3 || s.digits == 5) ? s.point * 10.0 : s.point;
    }
    // Place an order, returning the resulting ticket (0 on rejection).
    uint64_t send(const std::string& symbol, TradeAction action, double volume,
                  double price, double sl, double tp) {
        OrderRequest q; q.symbol = symbol; q.action = action; q.volume = volume;
        q.price = price; q.stopLoss = sl; q.takeProfit = tp;
        OrderResult r = service.orderSend(q);
        return r.ok() ? (uint64_t)r.order : 0;
    }
};

// ======================================================================================
// MT5Sugar — lifecycle / accessors
// ======================================================================================
MT5Sugar::MT5Sugar(const std::string& host, int port, const std::string& apiKey,
                   const std::string& name, bool utcTimes)
    : m_impl(new Impl(host, port, apiKey, name, utcTimes)) {}
MT5Sugar::~MT5Sugar() = default;

MT5Service& MT5Sugar::service() { return m_impl->service; }
MT5Account& MT5Sugar::account() { return m_impl->service.account(); }

#pragma region Connection
void MT5Sugar::connect(int64_t login, const std::string& password, const std::string& server,
                       const std::string& name, int timeoutSeconds) {
    m_impl->service.connect(login, password, server, name, timeoutSeconds);
}

// Read MT5_LOGIN / MT5_PASSWORD / MT5_SERVER from the environment and connect.
void MT5Sugar::quickConnect() {
    const char* login = std::getenv("MT5_LOGIN");
    const char* pass = std::getenv("MT5_PASSWORD");
    const char* server = std::getenv("MT5_SERVER");
    if (!login || !pass || !server)
        throw std::runtime_error("quickConnect: set MT5_LOGIN, MT5_PASSWORD and MT5_SERVER");
    connect(std::strtoll(login, nullptr, 10), pass, server);
}

bool MT5Sugar::isConnected() const { return m_impl->service.isConnected(); }

// Liveness: a successful account summary means the terminal answers.
bool MT5Sugar::ping() {
    try { m_impl->service.accountSummary(); return true; }
    catch (...) { return false; }
}

void MT5Sugar::disconnect() { m_impl->service.close(); }
#pragma endregion

#pragma region Balance / margin
double MT5Sugar::getBalance()     { return m_impl->service.accountInfoDouble(ACC_BALANCE); }
double MT5Sugar::getEquity()      { return m_impl->service.accountInfoDouble(ACC_EQUITY); }
double MT5Sugar::getMargin()      { return m_impl->service.accountInfoDouble(ACC_MARGIN); }
double MT5Sugar::getFreeMargin()  { return m_impl->service.accountInfoDouble(ACC_MARGIN_FREE); }
double MT5Sugar::getMarginLevel() { return m_impl->service.accountInfoDouble(ACC_MARGIN_LEVEL); }
double MT5Sugar::getProfit()      { return m_impl->service.accountInfoDouble(ACC_PROFIT); }
#pragma endregion

#pragma region Prices / quotes
double MT5Sugar::getBid(const std::string& symbol) { return m_impl->service.symbolInfoTick(symbol).bid; }
double MT5Sugar::getAsk(const std::string& symbol) { return m_impl->service.symbolInfoTick(symbol).ask; }
double MT5Sugar::getSpread(const std::string& symbol) {
    Tick t = m_impl->service.symbolInfoTick(symbol);
    return t.ask - t.bid;
}

PriceInfo MT5Sugar::getPriceInfo(const std::string& symbol) {
    Tick t = m_impl->service.symbolInfoTick(symbol);
    return PriceInfo{ symbol, t.bid, t.ask, t.ask - t.bid, t.time };
}

// Poll until the bid reaches targetBid (or any tick when targetBid == 0), up to timeoutMs.
PriceInfo MT5Sugar::waitForPrice(const std::string& symbol, double targetBid, int timeoutMs) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    PriceInfo last;
    while (std::chrono::steady_clock::now() < deadline) {
        Tick t = m_impl->service.symbolInfoTick(symbol);
        last = PriceInfo{ symbol, t.bid, t.ask, t.ask - t.bid, t.time };
        if (targetBid <= 0 || t.bid >= targetBid) return last;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    last.symbol.clear();   // timed out
    return last;
}
#pragma endregion

#pragma region Simple trading
uint64_t MT5Sugar::buyMarket(const std::string& symbol, double volume)  { return m_impl->send(symbol, TradeAction::Buy, volume, 0, 0, 0); }
uint64_t MT5Sugar::sellMarket(const std::string& symbol, double volume) { return m_impl->send(symbol, TradeAction::Sell, volume, 0, 0, 0); }
uint64_t MT5Sugar::buyLimit(const std::string& symbol, double volume, double price)  { return m_impl->send(symbol, TradeAction::BuyLimit, volume, price, 0, 0); }
uint64_t MT5Sugar::sellLimit(const std::string& symbol, double volume, double price) { return m_impl->send(symbol, TradeAction::SellLimit, volume, price, 0, 0); }
uint64_t MT5Sugar::buyStop(const std::string& symbol, double volume, double price)   { return m_impl->send(symbol, TradeAction::BuyStop, volume, price, 0, 0); }
uint64_t MT5Sugar::sellStop(const std::string& symbol, double volume, double price)  { return m_impl->send(symbol, TradeAction::SellStop, volume, price, 0, 0); }
#pragma endregion

#pragma region Trading with SL/TP
uint64_t MT5Sugar::buyMarketWithSLTP(const std::string& symbol, double volume, double sl, double tp)  { return m_impl->send(symbol, TradeAction::Buy, volume, 0, sl, tp); }
uint64_t MT5Sugar::sellMarketWithSLTP(const std::string& symbol, double volume, double sl, double tp) { return m_impl->send(symbol, TradeAction::Sell, volume, 0, sl, tp); }
uint64_t MT5Sugar::buyLimitWithSLTP(const std::string& symbol, double volume, double price, double sl, double tp)  { return m_impl->send(symbol, TradeAction::BuyLimit, volume, price, sl, tp); }
uint64_t MT5Sugar::sellLimitWithSLTP(const std::string& symbol, double volume, double price, double sl, double tp) { return m_impl->send(symbol, TradeAction::SellLimit, volume, price, sl, tp); }

// Convert pip distances to absolute SL/TP around the current price, then send.
uint64_t MT5Sugar::buyMarketWithPips(const std::string& symbol, double volume, double slPips, double tpPips) {
    SymbolInfo s = m_impl->service.symbolParams(symbol);
    double pip = m_impl->pipSize(s);
    Tick t = m_impl->service.symbolInfoTick(symbol);
    double sl = slPips > 0 ? t.ask - slPips * pip : 0;
    double tp = tpPips > 0 ? t.ask + tpPips * pip : 0;
    return m_impl->send(symbol, TradeAction::Buy, volume, 0, sl, tp);
}
uint64_t MT5Sugar::sellMarketWithPips(const std::string& symbol, double volume, double slPips, double tpPips) {
    SymbolInfo s = m_impl->service.symbolParams(symbol);
    double pip = m_impl->pipSize(s);
    Tick t = m_impl->service.symbolInfoTick(symbol);
    double sl = slPips > 0 ? t.bid + slPips * pip : 0;
    double tp = tpPips > 0 ? t.bid - tpPips * pip : 0;
    return m_impl->send(symbol, TradeAction::Sell, volume, 0, sl, tp);
}
#pragma endregion

#pragma region Position management
bool MT5Sugar::closePosition(int64_t ticket) {
    return m_impl->service.orderClose(ticket, 0, 10).ok();
}
bool MT5Sugar::closePositionPartial(int64_t ticket, double volume) {
    return m_impl->service.orderClose(ticket, volume, 10).ok();
}

// Close every open position; returns how many closed successfully.
int MT5Sugar::closeAllPositions() {
    auto positions = m_impl->service.openedOrders().positions;
    int closed = 0;
    for (const auto& p : positions) if (m_impl->service.orderClose(p.ticket, 0, 10).ok()) ++closed;
    return closed;
}

int MT5Sugar::closeAllBySymbol(const std::string& symbol) {
    auto positions = m_impl->service.openedOrders().positions;
    int closed = 0;
    for (const auto& p : positions)
        if (p.symbol == symbol && m_impl->service.orderClose(p.ticket, 0, 10).ok()) ++closed;
    return closed;
}

// Modify only the SL: look up the position's current TP so we don't clear it.
bool MT5Sugar::modifyPositionSL(int64_t ticket, double sl) {
    bool found = false; Position p = getPositionByTicket(ticket, found);
    double tp = found ? p.takeProfit : 0;
    return m_impl->service.orderModify(ticket, sl, tp).ok();
}
bool MT5Sugar::modifyPositionTP(int64_t ticket, double tp) {
    bool found = false; Position p = getPositionByTicket(ticket, found);
    double sl = found ? p.stopLoss : 0;
    return m_impl->service.orderModify(ticket, sl, tp).ok();
}
bool MT5Sugar::modifyPositionSLTP(int64_t ticket, double sl, double tp) {
    return m_impl->service.orderModify(ticket, sl, tp).ok();
}
#pragma endregion

#pragma region Position information
std::vector<Position> MT5Sugar::getOpenPositions() { return m_impl->service.openedOrders().positions; }

std::vector<Position> MT5Sugar::getPositionsBySymbol(const std::string& symbol) {
    std::vector<Position> out;
    for (const auto& p : m_impl->service.openedOrders().positions) if (p.symbol == symbol) out.push_back(p);
    return out;
}

Position MT5Sugar::getPositionByTicket(int64_t ticket, bool& found) {
    for (const auto& p : m_impl->service.openedOrders().positions)
        if (p.ticket == ticket) { found = true; return p; }
    found = false; return Position{};
}

bool MT5Sugar::hasOpenPosition(const std::string& symbol) {
    for (const auto& p : m_impl->service.openedOrders().positions) if (p.symbol == symbol) return true;
    return false;
}

int MT5Sugar::countOpenPositions() { return (int)m_impl->service.openedOrders().positions.size(); }

double MT5Sugar::getTotalProfit() {
    double sum = 0;
    for (const auto& p : m_impl->service.openedOrders().positions) sum += p.profit;
    return sum;
}

double MT5Sugar::getProfitBySymbol(const std::string& symbol) {
    double sum = 0;
    for (const auto& p : m_impl->service.openedOrders().positions) if (p.symbol == symbol) sum += p.profit;
    return sum;
}
#pragma endregion

#pragma region History / statistics
std::vector<PositionHistory> MT5Sugar::getDealsDateRange(int64_t fromUnix, int64_t toUnix) {
    return m_impl->service.positionsHistory(fromUnix, toUnix);
}
std::vector<PositionHistory> MT5Sugar::getDealsToday()     { return getDealsDateRange(todayStartUtc(), nowUnix()); }
std::vector<PositionHistory> MT5Sugar::getDealsYesterday() { int64_t s = todayStartUtc(); return getDealsDateRange(s - 86400, s); }
std::vector<PositionHistory> MT5Sugar::getDealsThisWeek()  { return getDealsDateRange(todayStartUtc() - 6 * 86400, nowUnix()); }
std::vector<PositionHistory> MT5Sugar::getDealsThisMonth() { return getDealsDateRange(todayStartUtc() - 29 * 86400, nowUnix()); }

static double sumProfit(const std::vector<PositionHistory>& v) {
    double s = 0; for (const auto& p : v) s += p.profit + p.swap + p.commission + p.fee; return s;
}
double MT5Sugar::getProfitToday()     { return sumProfit(getDealsToday()); }
double MT5Sugar::getProfitThisWeek()  { return sumProfit(getDealsThisWeek()); }
double MT5Sugar::getProfitThisMonth() { return sumProfit(getDealsThisMonth()); }

// Today's win/loss summary from the closed positions.
DailyStats MT5Sugar::getDailyStats() {
    auto deals = getDealsToday();
    DailyStats st;
    for (const auto& d : deals) {
        double net = d.profit + d.swap + d.commission + d.fee;
        st.totalDeals++;
        st.totalProfit += net;
        if (net >= 0) st.winningDeals++; else st.losingDeals++;
        st.bestDeal = std::max(st.bestDeal, net);
        st.worstDeal = std::min(st.worstDeal, net);
    }
    st.winRate = st.totalDeals ? (100.0 * st.winningDeals / st.totalDeals) : 0;
    return st;
}
#pragma endregion

#pragma region Symbol information
SymbolInfo MT5Sugar::getSymbolInfo(const std::string& symbol) { return m_impl->service.symbolParams(symbol); }
std::vector<std::string> MT5Sugar::getAllSymbols() { return m_impl->service.symbolList(); }

bool MT5Sugar::isSymbolAvailable(const std::string& symbol) {
    try { return m_impl->service.symbolExist(symbol); } catch (...) { return false; }
}

int64_t MT5Sugar::getMinStopLevel(const std::string& symbol) {
    return (int64_t)m_impl->service.symbolParams(symbol).stopsLevel;
}
int MT5Sugar::getSymbolDigits(const std::string& symbol) {
    return m_impl->service.symbolParams(symbol).digits;
}
#pragma endregion

#pragma region Risk management
// Margin to open `volume` lots on the buy side at the current ask.
double MT5Sugar::calculateRequiredMargin(const std::string& symbol, double volume) {
    double ask = m_impl->service.symbolInfoTick(symbol).ask;
    return m_impl->service.orderCalcMargin(symbol, ORDER_TF_BUY, volume, ask);
}

// Lot size so that `stopLossPips` risks `riskPercent`% of the balance, clamped to the symbol's
// volume min/max and rounded down to its volume step.
double MT5Sugar::calculatePositionSize(const std::string& symbol, double riskPercent, double stopLossPips) {
    if (stopLossPips <= 0) return 0;
    SymbolInfo s = m_impl->service.symbolParams(symbol);
    double pip = m_impl->pipSize(s);
    double valuePerPipPerLot = (s.tickSize > 0) ? s.tickValue * (pip / s.tickSize) : s.tickValue;
    if (valuePerPipPerLot <= 0) return 0;
    double riskAmount = getBalance() * riskPercent / 100.0;
    double lot = riskAmount / (stopLossPips * valuePerPipPerLot);
    if (s.volumeStep > 0) lot = std::floor(lot / s.volumeStep) * s.volumeStep;
    lot = std::max(s.volumeMin, std::min(lot, s.volumeMax));
    return lot;
}

// Biggest lot the free margin can open now, rounded down to the volume step.
double MT5Sugar::getMaxLotSize(const std::string& symbol) {
    double marginPerLot = calculateRequiredMargin(symbol, 1.0);
    if (marginPerLot <= 0) return 0;
    SymbolInfo s = m_impl->service.symbolParams(symbol);
    double lot = getFreeMargin() / marginPerLot;
    if (s.volumeStep > 0) lot = std::floor(lot / s.volumeStep) * s.volumeStep;
    return std::max(0.0, std::min(lot, s.volumeMax));
}

bool MT5Sugar::canOpenPosition(const std::string& symbol, double volume, std::string& reason) {
    double need = calculateRequiredMargin(symbol, volume);
    double free = getFreeMargin();
    if (need > free) {
        reason = "insufficient free margin (need " + std::to_string(need) + ", have " + std::to_string(free) + ")";
        return false;
    }
    reason.clear();
    return true;
}

// Absolute SL/TP prices for a "buy"/"sell" around entryPrice given pip distances.
void MT5Sugar::calculateSLTP(const std::string& symbol, const std::string& direction, double entryPrice,
                             double stopLossPips, double takeProfitPips, double& slOut, double& tpOut) {
    SymbolInfo s = m_impl->service.symbolParams(symbol);
    double pip = m_impl->pipSize(s);
    const bool buy = (direction == "buy" || direction == "BUY" || direction == "Buy");
    if (buy) {
        slOut = stopLossPips > 0 ? entryPrice - stopLossPips * pip : 0;
        tpOut = takeProfitPips > 0 ? entryPrice + takeProfitPips * pip : 0;
    } else {
        slOut = stopLossPips > 0 ? entryPrice + stopLossPips * pip : 0;
        tpOut = takeProfitPips > 0 ? entryPrice - takeProfitPips * pip : 0;
    }
}
#pragma endregion

#pragma region Account information
AccountInfo MT5Sugar::getAccountInfo() { return m_impl->service.accountSummary(); }
#pragma endregion

} // namespace metarpc
