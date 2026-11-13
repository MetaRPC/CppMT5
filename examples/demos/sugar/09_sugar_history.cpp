/*══════════════════════════════════════════════════════════════════════════════
 FILE: 09_sugar_history.cpp - SUGAR HISTORY & STATISTICS

 PURPOSE:
   Read-only history helpers: closed deals over common windows and a quick daily
   win/loss summary — one call each, UTC day boundaries handled for you.

 📚 WHAT THIS DEMO COVERS:
   • getDealsToday / Yesterday / ThisWeek / ThisMonth -> vector<PositionHistory>
   • getProfitToday / ThisWeek / ThisMonth            -> net profit
   • getDailyStats()                                  -> DailyStats (win rate, best/worst)

 🔄 API LEVELS:  HIGH-LEVEL (Sugar) — THIS FILE.

 🚀 HOW TO RUN:  ./build/demos 9
══════════════════════════════════════════════════════════════════════════════*/

#include <iostream>

#include "metarpc/mt5.hpp"
#include "config/config.hpp"
#include "helpers/connection.hpp"

namespace demos {

using namespace metarpc;

void demo_sugar_history(MT5Sugar& sugar, const Config& /*cfg*/) {
    printSub("Closed deals by window");
    std::cout << "  today      : " << sugar.getDealsToday().size()     << " deals,  profit " << sugar.getProfitToday()     << "\n";
    std::cout << "  yesterday  : " << sugar.getDealsYesterday().size() << " deals\n";
    std::cout << "  this week  : " << sugar.getDealsThisWeek().size()  << " deals,  profit " << sugar.getProfitThisWeek()  << "\n";
    std::cout << "  this month : " << sugar.getDealsThisMonth().size() << " deals,  profit " << sugar.getProfitThisMonth() << "\n";

    printSub("Today's statistics");
    DailyStats st = sugar.getDailyStats();
    std::cout << "  total deals  : " << st.totalDeals << "\n";
    std::cout << "  winning      : " << st.winningDeals << "\n";
    std::cout << "  losing       : " << st.losingDeals << "\n";
    std::cout << "  win rate     : " << st.winRate << " %\n";
    std::cout << "  total profit : " << st.totalProfit << "\n";
    std::cout << "  best / worst : " << st.bestDeal << " / " << st.worstDeal << "\n";

    printSub("Most recent closed deals (up to 5)");
    auto deals = sugar.getDealsThisMonth();
    int shown = 0;
    for (auto it = deals.rbegin(); it != deals.rend() && shown < 5; ++it, ++shown)
        std::cout << "  #" << it->ticket << "  " << it->symbol
                  << "  vol=" << it->volume << "  profit=" << it->profit
                  << "  closed t=" << it->closeTime << "\n";
    if (deals.empty()) std::cout << "  (no closed deals this month)\n";
}

} // namespace demos
