/*══════════════════════════════════════════════════════════════════════════════
 FILE: quickstart.cpp - the smallest end-to-end CppMT5 program

 PURPOSE:
   Connect by broker server name, read the account, quote a symbol, then place and
   close one tiny market order on a DEMO account — using the high-level MT5Sugar API.

 ⚠ SAFETY: places a real (tiny, self-closing) order; it refuses to run on a non-DEMO account.

 🚀 HOW TO RUN:
   export MRPC_API_KEY=mrpc_live_...
   export MT5_LOGIN=12345678 MT5_PASSWORD=... MT5_SERVER=MetaQuotes-Demo
   cmake -B build -DBUILD_EXAMPLES=ON && cmake --build build --target quickstart
   ./build/quickstart
══════════════════════════════════════════════════════════════════════════════*/

#include "metarpc/mt5_sugar.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    using namespace metarpc;

    // Credentials come from the environment (see the header above).
    const char* login    = std::getenv("MT5_LOGIN");
    const char* password = std::getenv("MT5_PASSWORD");
    const char* server   = std::getenv("MT5_SERVER");
    if (!login || !password || !server) {
        std::cerr << "set MT5_LOGIN, MT5_PASSWORD, MT5_SERVER (and MRPC_API_KEY)\n";
        return 2;
    }

    // Construct the high-level layer. The API key defaults to $MRPC_API_KEY (else "TRIAL").
    MT5Sugar sugar("mt5.mrpc.pro", 443);

    try {
        // ──────────────────────────────────────────────────────────────────────
        //  1  connect(login, password, server)
        //     Starts/attaches the terminal; the session id is obtained and stored
        //     automatically (no manual GetId). Throws on failure.
        // ──────────────────────────────────────────────────────────────────────
        sugar.connect(std::atoll(login), password, server, "cppmt5-quickstart");

        // ──────────────────────────────────────────────────────────────────────
        //  2  getAccountInfo()  → AccountInfo
        //     Full account snapshot. Refuse to trade anywhere but a DEMO account.
        // ──────────────────────────────────────────────────────────────────────
        AccountInfo acc = sugar.getAccountInfo();
        std::cout << "Account " << acc.login << " (" << acc.name << ") " << acc.balance << " "
                  << acc.currency << "  leverage 1:" << acc.leverage << "\n";
        if (acc.tradeMode != TradeMode::Demo) {
            std::cerr << "refusing to trade on a non-DEMO account\n";
            return 3;
        }

        const std::string symbol = "EURUSD";

        // ──────────────────────────────────────────────────────────────────────
        //  3  getSymbolInfo(symbol) → SymbolInfo   and   getPriceInfo(symbol) → PriceInfo
        //     Symbol spec (digits, volume limits) and the current bid/ask.
        // ──────────────────────────────────────────────────────────────────────
        SymbolInfo info = sugar.getSymbolInfo(symbol);
        PriceInfo  px   = sugar.getPriceInfo(symbol);
        std::cout << symbol << "  bid=" << px.bid << " ask=" << px.ask
                  << "  digits=" << info.digits << " volMin=" << info.volumeMin << "\n";

        // ──────────────────────────────────────────────────────────────────────
        //  4  buyMarket(symbol, volume)  ⚠ places a REAL (tiny) order
        //     Returns the ticket, or 0 if rejected.
        // ──────────────────────────────────────────────────────────────────────
        uint64_t ticket = sugar.buyMarket(symbol, info.volumeMin);
        if (ticket == 0) { std::cerr << "order rejected (market closed / no margin?)\n"; return 1; }
        std::cout << "opened #" << ticket << "\n";

        // ──────────────────────────────────────────────────────────────────────
        //  5  closePosition(ticket)  ⚠ closes the position we just opened
        // ──────────────────────────────────────────────────────────────────────
        std::cout << "close -> " << (sugar.closePosition((int64_t)ticket) ? "ok" : "FAILED") << "\n";
    }
    catch (const ApiError& e)       { std::cerr << "API error: " << e.what() << "\n";       return 1; }
    catch (const TransportError& e) { std::cerr << "transport error: " << e.what() << "\n"; return 1; }

    // No disconnect(): the terminal stays alive on the gateway for the next run.
    return 0;
}
