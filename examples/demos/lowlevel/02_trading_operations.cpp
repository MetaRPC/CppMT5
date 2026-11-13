/*══════════════════════════════════════════════════════════════════════════════
 FILE: 02_trading_operations.cpp - LOW-LEVEL MT5Account TRADE CALC & CHECK

 PURPOSE:
   The raw pre-trade RPCs: compute margin/profit and dry-run validate an order with
   the protobuf API directly. Read-only — it does NOT place any order (see demo [7]
   for an actual Sugar round-trip).

 📚 WHAT THIS DEMO COVERS:
   • orderCalcMargin()  -> margin required for a hypothetical order
   • orderCalcProfit()  -> profit for a hypothetical price move
   • orderCheck()       -> MQL5-style dry-run validation (retcode, free margin, …)
   (to actually trade: orderSend / orderModify / orderClose)

 🔄 API LEVELS:
   LOW-LEVEL (Account) - THIS FILE: raw protobuf trade functions
   HIGH-LEVEL (Sugar)  - buyMarket()/calculateRequiredMargin() → sugar

 🚀 HOW TO RUN:  ./build/demos 2
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

namespace pb = mt5_term_api;   // protobuf types for the raw layer

void demo_trading_operations(metarpc::MT5Sugar& sugar, const Config& cfg) {
    metarpc::MT5Account& acc = sugar.account();
    const std::string sym = cfg.testSymbol;
    const double vol = cfg.testVolume;

    // ──────────────────────────────────────────────────────────────────────────
    //  0  symbolInfoTick(symbol)  — current price for the calculations below
    //     Returns: MrpcMqlTick → ask().
    // ──────────────────────────────────────────────────────────────────────────
    pb::SymbolInfoTickRequest treq; treq.set_symbol(sym);
    pb::MrpcMqlTick tick = acc.symbolInfoTick(treq);
    const double ask = tick.ask();
    std::cout << "  " << sym << " ask = " << ask << ",  volume = " << vol << "\n";

    // ── orderCalcMargin ──────────────────────────────────────────────────────
    printSub("orderCalcMargin");

    // ──────────────────────────────────────────────────────────────────────────
    //  1  orderCalcMargin(symbol, order_type, volume, open_price)
    //     Margin a prospective order would require — an affordability pre-check.
    //     Returns: OrderCalcMarginData → margin() (account currency).
    // ──────────────────────────────────────────────────────────────────────────
    pb::OrderCalcMarginRequest mreq;
    mreq.set_symbol(sym); mreq.set_order_type((pb::ENUM_ORDER_TYPE_TF)0 /*BUY*/);
    mreq.set_volume(vol); mreq.set_open_price(ask);
    std::cout << "  margin for BUY " << vol << " " << sym << " : " << acc.orderCalcMargin(mreq).margin() << "\n";

    // ── orderCalcProfit ──────────────────────────────────────────────────────
    printSub("orderCalcProfit (+50 pips)");

    // ──────────────────────────────────────────────────────────────────────────
    //  2  orderCalcProfit(symbol, order_type, volume, open_price, close_price)
    //     Profit a given price move would yield — size trades by reward.
    //     Returns: OrderCalcProfitData → profit() (can be negative).
    // ──────────────────────────────────────────────────────────────────────────
    pb::OrderCalcProfitRequest preq;
    preq.set_symbol(sym); preq.set_order_type((pb::ENUM_ORDER_TYPE_TF)0 /*BUY*/);
    preq.set_volume(vol); preq.set_open_price(ask); preq.set_close_price(ask + 0.0050);
    std::cout << "  profit if price +50 pips : " << acc.orderCalcProfit(preq).profit() << "\n";

    // ── orderCheck (dry run) ─────────────────────────────────────────────────
    printSub("orderCheck (dry-run validation)");

    // ──────────────────────────────────────────────────────────────────────────
    //  3  orderCheck(mql_trade_request)
    //     Dry-run an order WITHOUT sending it: margin, free margin and a retcode.
    //     Returns: OrderCheckData → mql_trade_check_result() (retcode 0 = would pass).
    // ──────────────────────────────────────────────────────────────────────────
    pb::OrderCheckRequest creq;
    auto* tr = creq.mutable_mql_trade_request();
    tr->set_action((pb::MRPC_ENUM_TRADE_REQUEST_ACTIONS)0 /*DEAL*/);
    tr->set_symbol(sym); tr->set_volume(vol);
    tr->set_order_type((pb::ENUM_ORDER_TYPE_TF)0 /*BUY*/); tr->set_price(ask);
    const auto& r = acc.orderCheck(creq).mql_trade_check_result();
    std::cout << "  retcode      : " << r.returned_code() << (r.returned_code() == 0 ? "  (would pass)" : "") << "\n";
    std::cout << "  balance after: " << r.balance_after_deal() << "\n";
    std::cout << "  margin       : " << r.margin() << "\n";
    std::cout << "  free margin  : " << r.free_margin() << "\n";
    std::cout << "  comment      : " << r.comment() << "\n";
}

} // namespace demos
