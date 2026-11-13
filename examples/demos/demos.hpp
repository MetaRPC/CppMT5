#pragma once
// Registry of demo entry points. Each demo takes an already-connected MT5Sugar (which exposes
// all three layers via sugar / sugar.service() / sugar.account()) and the loaded Config.
// main.cpp wires menu numbers to these.

#include "metarpc/mt5_sugar.hpp"
#include "config/config.hpp"

namespace demos {

// Low-level (MT5Account, raw protobuf)
void demo_general_operations(metarpc::MT5Sugar& sugar, const Config& cfg);   // [1]
void demo_trading_operations(metarpc::MT5Sugar& sugar, const Config& cfg);   // [2]
void demo_streaming_methods(metarpc::MT5Sugar& sugar, const Config& cfg);    // [3]

// Mid-level (MT5Service, friendly structs)
void demo_service_demo(metarpc::MT5Sugar& sugar, const Config& cfg);         // [4]
void demo_service_streaming(metarpc::MT5Sugar& sugar, const Config& cfg);    // [5]

// High-level (MT5Sugar)
void demo_sugar_basics(metarpc::MT5Sugar& sugar, const Config& cfg);         // [6]
void demo_sugar_trading(metarpc::MT5Sugar& sugar, const Config& cfg);        // [7]  (places orders)
void demo_sugar_positions(metarpc::MT5Sugar& sugar, const Config& cfg);      // [8]
void demo_sugar_history(metarpc::MT5Sugar& sugar, const Config& cfg);        // [9]
void demo_sugar_advanced(metarpc::MT5Sugar& sugar, const Config& cfg);       // [10]

// Orchestrators (strategy managers built on Sugar)
void demo_trailing_stop(metarpc::MT5Sugar& sugar, const Config& cfg);        // [11] (places orders)
void demo_grid_trader(metarpc::MT5Sugar& sugar, const Config& cfg);          // [13] (places orders)
void demo_risk_manager(metarpc::MT5Sugar& sugar, const Config& cfg);         // [14]

// Presets (coordinate orchestrators)
void demo_adaptive_preset(metarpc::MT5Sugar& sugar, const Config& cfg);      // [16] (may place orders)

} // namespace demos
