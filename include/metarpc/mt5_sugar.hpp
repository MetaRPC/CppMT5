#pragma once
// MetaRPC CppMT5 — Layer 3 of 3: MT5Sugar (high-level convenience).
//
// MT5Sugar owns an MT5Service (Layer 2) and offers the shortest possible one-liners for the
// everyday tasks a strategy needs: "what's my balance?", "what's the bid?", "buy 0.1 lots",
// "close everything". Every method is self-contained — no context, no request objects, no
// protobuf — so prototyping code reads like plain English. When you outgrow it, drop down to
// service() for the friendly-typed facade or account() for raw RPCs.
//
// Like MT5Service, this header is protobuf-free and compiles fast.

#include <cstdint>
#include <string>
#include <vector>

#include "metarpc/mt5_service.hpp"   // MT5Service + the friendly structs/enums

namespace metarpc {

#pragma region Sugar structs
// ======================================================================================
// Small result structs specific to the sugar layer
// ======================================================================================

/// Compact price snapshot, from getPriceInfo().
struct PriceInfo {
    std::string symbol;
    double bid{0};
    double ask{0};
    double spread{0};       ///< ask - bid, in price units
    int64_t time{0};        ///< tick time (UTC seconds)
};

/// A day's trading summary, from getDailyStats().
struct DailyStats {
    int totalDeals{0};
    int winningDeals{0};
    int losingDeals{0};
    double winRate{0};      ///< winning / total, in percent
    double totalProfit{0};  ///< net profit (incl. swap & commission)
    double bestDeal{0};
    double worstDeal{0};
};
#pragma endregion

// ======================================================================================
// MT5Sugar — Layer 3 (high-level, no context, no protobuf)
// ======================================================================================
class MT5Sugar {
public:
    /// Construct the sugar layer and the MT5Service (and MT5Account) it owns. Call connect()
    /// or quickConnect() before trading. Same connection args as the lower layers.
    explicit MT5Sugar(const std::string& host = "mt5.mrpc.pro", int port = 443,
                      const std::string& apiKey = "", const std::string& name = "",
                      bool utcTimes = true);
    ~MT5Sugar();
    MT5Sugar(const MT5Sugar&) = delete;
    MT5Sugar& operator=(const MT5Sugar&) = delete;

    /// The facade layer underneath (friendly types, full API).
    MT5Service& service();
    /// The low-level layer (raw protobuf RPCs).
    MT5Account& account();

#pragma region Connection
    // --- connection ---------------------------------------------------------------------

    /// Connect by login/password/server (same as service().connect()).
    void connect(int64_t login, const std::string& password, const std::string& server,
                 const std::string& name = "", int timeoutSeconds = 120);
    /// One-liner connect that reads credentials from the environment variables
    /// MT5_LOGIN / MT5_PASSWORD / MT5_SERVER. @throws std::runtime_error if any is missing.
    void quickConnect();
    bool isConnected() const;                   ///< true after a successful connect
    /// Lightweight liveness check: fetches the account summary and returns true if it succeeds.
    bool ping();
    void disconnect();                          ///< drop the channel (keeps the terminal alive)
#pragma endregion

#pragma region Balance / margin
    // --- balance / margin ---------------------------------------------------------------

    double getBalance();        ///< account balance
    double getEquity();         ///< account equity (balance + floating P/L)
    double getMargin();         ///< used margin
    double getFreeMargin();     ///< free margin
    double getMarginLevel();    ///< margin level, %
    double getProfit();         ///< current floating profit
#pragma endregion

#pragma region Prices / quotes
    // --- prices / quotes ----------------------------------------------------------------

    double getBid(const std::string& symbol);       ///< current bid
    double getAsk(const std::string& symbol);       ///< current ask
    double getSpread(const std::string& symbol);    ///< ask - bid (price units)
    PriceInfo getPriceInfo(const std::string& symbol);  ///< bid/ask/spread/time in one call
    /// Poll until the bid is at/beyond `targetBid` (or `targetBid` is 0 for any tick), up to
    /// `timeoutMs`. Returns the last price seen; empty symbol on timeout.
    PriceInfo waitForPrice(const std::string& symbol, double targetBid, int timeoutMs = 10000);
#pragma endregion

#pragma region Simple trading
    // --- simple trading (return the resulting ticket, 0 on a rejected order) --------------

    uint64_t buyMarket(const std::string& symbol, double volume);
    uint64_t sellMarket(const std::string& symbol, double volume);
    uint64_t buyLimit(const std::string& symbol, double volume, double price);
    uint64_t sellLimit(const std::string& symbol, double volume, double price);
    uint64_t buyStop(const std::string& symbol, double volume, double price);
    uint64_t sellStop(const std::string& symbol, double volume, double price);
#pragma endregion

#pragma region Trading with SL/TP
    // --- market orders with stop loss / take profit -------------------------------------

    uint64_t buyMarketWithSLTP(const std::string& symbol, double volume, double sl, double tp);
    uint64_t sellMarketWithSLTP(const std::string& symbol, double volume, double sl, double tp);
    /// Pending buy-limit at `price` with absolute SL/TP.
    uint64_t buyLimitWithSLTP(const std::string& symbol, double volume, double price, double sl, double tp);
    /// Pending sell-limit at `price` with absolute SL/TP.
    uint64_t sellLimitWithSLTP(const std::string& symbol, double volume, double price, double sl, double tp);
    /// Buy at market with SL/TP expressed as a distance in pips from entry.
    uint64_t buyMarketWithPips(const std::string& symbol, double volume, double slPips, double tpPips);
    uint64_t sellMarketWithPips(const std::string& symbol, double volume, double slPips, double tpPips);
#pragma endregion

#pragma region Position management
    // --- position management ------------------------------------------------------------

    bool closePosition(int64_t ticket);                          ///< close a whole position
    bool closePositionPartial(int64_t ticket, double volume);    ///< close part of a position
    int closeAllPositions();                                     ///< close every position; returns count closed
    int closeAllBySymbol(const std::string& symbol);             ///< close all positions on a symbol
    bool modifyPositionSL(int64_t ticket, double sl);            ///< set stop loss only
    bool modifyPositionTP(int64_t ticket, double tp);            ///< set take profit only
    bool modifyPositionSLTP(int64_t ticket, double sl, double tp);
#pragma endregion

#pragma region Position information
    // --- position information -----------------------------------------------------------

    std::vector<Position> getOpenPositions();                        ///< all open positions
    std::vector<Position> getPositionsBySymbol(const std::string& symbol);
    /// The position with this ticket; `found` is set false if there is none.
    Position getPositionByTicket(int64_t ticket, bool& found);
    bool hasOpenPosition(const std::string& symbol);                 ///< any position on the symbol?
    int countOpenPositions();                                        ///< number of open positions
    double getTotalProfit();                                         ///< summed floating profit
    double getProfitBySymbol(const std::string& symbol);            ///< floating profit on one symbol
#pragma endregion

#pragma region History / statistics
    // --- history / statistics (UTC day boundaries) --------------------------------------

    std::vector<PositionHistory> getDealsToday();
    std::vector<PositionHistory> getDealsYesterday();
    std::vector<PositionHistory> getDealsThisWeek();
    std::vector<PositionHistory> getDealsThisMonth();
    std::vector<PositionHistory> getDealsDateRange(int64_t fromUnix, int64_t toUnix);
    double getProfitToday();
    double getProfitThisWeek();
    double getProfitThisMonth();
    DailyStats getDailyStats();     ///< today's win/loss summary
#pragma endregion

#pragma region Symbol information
    // --- symbol information -------------------------------------------------------------

    SymbolInfo getSymbolInfo(const std::string& symbol);    ///< full params for a symbol
    std::vector<std::string> getAllSymbols();               ///< every symbol name
    bool isSymbolAvailable(const std::string& symbol);      ///< exists and is selectable
    int64_t getMinStopLevel(const std::string& symbol);     ///< min SL/TP distance, in points
    int getSymbolDigits(const std::string& symbol);         ///< price decimal places
#pragma endregion

#pragma region Risk management
    // --- risk management ----------------------------------------------------------------

    /// Lot size so that a `stopLossPips` stop risks `riskPercent`% of the balance.
    double calculatePositionSize(const std::string& symbol, double riskPercent, double stopLossPips);
    /// Largest lot the current free margin can open on this symbol.
    double getMaxLotSize(const std::string& symbol);
    /// Whether a `volume`-lot order can be opened now; `reason` explains a false.
    bool canOpenPosition(const std::string& symbol, double volume, std::string& reason);
    /// Margin required to open `volume` lots on this symbol (buy side).
    double calculateRequiredMargin(const std::string& symbol, double volume);
    /// Absolute SL/TP prices for a direction ("buy"/"sell") given pip distances from entry.
    void calculateSLTP(const std::string& symbol, const std::string& direction, double entryPrice,
                       double stopLossPips, double takeProfitPips, double& slOut, double& tpOut);
#pragma endregion

#pragma region Account information
    // --- account information ------------------------------------------------------------

    AccountInfo getAccountInfo();   ///< full account snapshot
#pragma endregion

    struct Impl;
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace metarpc
