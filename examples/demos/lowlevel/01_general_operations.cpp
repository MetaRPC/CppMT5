/*══════════════════════════════════════════════════════════════════════════════
 FILE: 01_general_operations.cpp - LOW-LEVEL MT5Account GENERAL OPERATIONS

 PURPOSE:
   Shows the raw, 1:1-with-the-wire layer: build a protobuf *Request, call the
   matching MT5Account method, read fields off the returned *Data. This is the most
   flexible (and most verbose) way to use the SDK.

 🎯 WHO SHOULD USE THIS:
   • You need a field the friendly layers do not expose
   • You want full control over the exact request sent

 📚 WHAT THIS DEMO COVERS (2 categories):

   1. ACCOUNT (raw)
      • accountSummary()      -> AccountSummaryData (balance/equity/leverage/…)
      • accountInfoDouble()   -> one double property by enum id
      • accountInfoInteger()  -> one integer property by enum id

   2. SYMBOLS (raw)
      • symbolsTotal()        -> number of symbols
      • symbolName()          -> symbol name by index
      • symbolExist()         -> does the broker know a symbol
      • symbolInfoTick()      -> latest tick (MrpcMqlTick)
      • symbolInfoInteger()   -> digits
      • symbolParamsMany()    -> full params for a symbol

 🔄 API LEVELS:
   LOW-LEVEL (Account)  - THIS FILE: raw protobuf, maximum flexibility
   MID-LEVEL (Service)  - same data as friendly structs            → sugar.service()
   HIGH-LEVEL (Sugar)   - one-liners                               → sugar

 🚀 HOW TO RUN:  ./build/demos 1
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

namespace pb = mt5_term_api;   // protobuf types for the raw layer

void demo_general_operations(metarpc::MT5Sugar& sugar, const Config& cfg) {
    metarpc::MT5Account& acc = sugar.account();   // the low-level layer
    const std::string sym = cfg.testSymbol;

    // ── 1. ACCOUNT (raw protobuf) ────────────────────────────────────────────
    printSub("Account (raw)");
    pb::AccountSummaryData s = acc.accountSummary();
    std::cout << "  login        : " << s.account_login() << "\n";
    std::cout << "  balance      : " << s.account_balance() << " " << s.account_currency() << "\n";
    std::cout << "  equity       : " << s.account_equity() << "\n";
    std::cout << "  leverage     : 1:" << s.account_leverage() << "\n";
    std::cout << "  company      : " << s.account_company_name() << "\n";

    // AccountInfoDoublePropertyType: 0 BALANCE, 5 MARGIN_FREE; IntegerType: 0 LOGIN
    pb::AccountInfoDoubleRequest dreq; dreq.set_property_id((pb::AccountInfoDoublePropertyType)5);
    std::cout << "  free margin  : " << acc.accountInfoDouble(dreq).requested_value() << "\n";
    pb::AccountInfoIntegerRequest ireq; ireq.set_property_id((pb::AccountInfoIntegerPropertyType)0);
    std::cout << "  login (int)  : " << acc.accountInfoInteger(ireq).requested_value() << "\n";

    // ── 2. SYMBOLS (raw protobuf) ────────────────────────────────────────────
    printSub("Symbols (raw)");
    pb::SymbolsTotalRequest streq; streq.set_mode(false);
    int total = acc.symbolsTotal(streq).total();
    std::cout << "  symbols total: " << total << "\n";

    if (total > 0) {
        pb::SymbolNameRequest nreq; nreq.set_index(0); nreq.set_selected(false);
        std::cout << "  symbol[0]    : " << acc.symbolName(nreq).name() << "\n";
    }

    pb::SymbolExistRequest ereq; ereq.set_name(sym);
    std::cout << "  '" << sym << "' exists: " << (acc.symbolExist(ereq).exists() ? "yes" : "no") << "\n";

    pb::SymbolInfoTickRequest treq; treq.set_symbol(sym);
    pb::MrpcMqlTick tick = acc.symbolInfoTick(treq);
    std::cout << "  " << sym << " tick : bid=" << tick.bid() << " ask=" << tick.ask() << "\n";

    // SymbolInfoIntegerProperty: 17 = SYMBOL_DIGITS
    pb::SymbolInfoIntegerRequest sireq; sireq.set_symbol(sym); sireq.set_type((pb::SymbolInfoIntegerProperty)17);
    std::cout << "  " << sym << " digits: " << acc.symbolInfoInteger(sireq).value() << "\n";

    pb::SymbolParamsManyRequest preq; preq.set_symbol_name(sym);
    pb::SymbolParamsManyData params = acc.symbolParamsMany(preq);
    if (params.symbol_infos_size() > 0) {
        const auto& p = params.symbol_infos(0);
        std::cout << "  " << sym << " point=" << p.point()
                  << " vol[min=" << p.volume_min() << " max=" << p.volume_max()
                  << " step=" << p.volume_step() << "]\n";
    }
}

} // namespace demos
