#pragma once
// MetaRPC CppMT5 — umbrella header.
//
// Pulls in all three layers so application code can `#include <metarpc/mt5.hpp>` and use
// whichever level it needs:
//
//   metarpc::MT5Sugar    — Layer 3, high-level one-liners (getBalance, buyMarket, …)
//   metarpc::MT5Service  — Layer 2, friendly-typed facade (AccountInfo, Tick, Position, …)
//   metarpc::MT5Account  — Layer 1, raw protobuf RPCs (pulls in the generated headers)
//
// Prefer including only the layer you use (mt5_service.hpp / mt5_sugar.hpp are protobuf-free
// and compile fast); include this umbrella for convenience or when you need all three.

#include "metarpc/mt5_common.hpp"    // ApiError, TransportError, ClientOptions
#include "metarpc/mt5_service.hpp"   // friendly structs/enums + MT5Service
#include "metarpc/mt5_sugar.hpp"     // MT5Sugar
#include "metarpc/mt5_account.hpp"   // MT5Account (protobuf)
