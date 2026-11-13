# CppMT5 SDK

> High-performance, modern **C++17** SDK for MetaTrader 5 algorithmic trading automation over gRPC.

[![Docs](https://img.shields.io/badge/docs-CppMT5-blue.svg)](https://metarpc.github.io/CppMT5/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

📄 **Full documentation**: [https://metarpc.github.io/CppMT5/](https://metarpc.github.io/CppMT5/)

CppMT5 connects to real MetaTrader 5 terminals through the MetaRPC gRPC gateway — read the account, stream live prices, and place/manage orders from C++.

---

## 🧱 Three-layer architecture

| Layer | Type | What it gives you |
|-------|------|-------------------|
| Low | `MT5Account` | Raw gRPC: builds each proto `…Request`, attaches auth metadata, returns typed `…Data` payloads (or throws) |
| Mid | `MT5Service` | Friendly args and named C++ structs, times in UTC — a **protobuf-free** header |
| High | `MT5Sugar` | One-liners: balances, prices, order construction, pip-based SL/TP, risk sizing |

Each layer **owns** the one below (`sugar.service()`, `service.account()`); ready [Strategies](https://metarpc.github.io/CppMT5/Strategies/Strategies.Master.Overview/) build on top. Errors are exceptions (`ApiError` / `TransportError`); transient failures retry and a dropped terminal re-attaches automatically.

---

## 📦 Build

Requirements: a **C++17** compiler, **CMake ≥ 3.15** (Ninja recommended), and the **protobuf** + **grpc++** development packages (discovered via `pkg-config`).

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake ninja-build pkg-config \
     libprotobuf-dev protobuf-compiler libgrpc++-dev protobuf-compiler-grpc

# MSYS2 (Windows, UCRT64)
pacman -S mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,pkgconf,protobuf,grpc}

# macOS (Homebrew)
brew install cmake ninja pkg-config protobuf grpc
```

```bash
git clone https://github.com/MetaRPC/CppMT5.git
cd CppMT5
cmake -G Ninja -B build
cmake --build build
```

This produces the `metarpc_cppmt5` static library plus the **`demos`** launcher and the offline **`protobuf_inspector`**. The gRPC contract ships as one version-independent file, [`proto/mt5_term_api.desc`](https://github.com/MetaRPC/CppMT5/blob/main/proto/mt5_term_api.desc); CMake runs **your** `protoc` on it at build time, so the generated stubs always match your installed protobuf — download that one file to build a client in any language.

---

## 🚀 Minimal example

```cpp
#include <iostream>
#include <metarpc/mt5_sugar.hpp>
using namespace metarpc;

int main() {
    // host, port (API key read from MRPC_API_KEY, or pass it to the constructor)
    MT5Sugar sugar("mt5.mrpc.pro", 443);

    sugar.connect(5057947437, "password", "MetaQuotes-Demo");   // exact broker server name
    std::cout << "balance " << sugar.getBalance()
              << "  bid " << sugar.getBid("EURUSD") << "\n";
    sugar.disconnect();
}
```

Link it:

```cmake
add_subdirectory(CppMT5)
target_link_libraries(my_app PRIVATE metarpc_cppmt5)
```

---

## 🏃 Running the examples

Settings come from `examples/demos/config/config.json` **and/or** environment variables — **env vars override the file**:

```json
{ "user": 5057947437, "password": "YourPassword", "grpc_server": "mt5.mrpc.pro:443",
  "mt_cluster": "MetaQuotes-Demo", "api_key": "", "test_symbol": "EURUSD", "test_volume": 0.01 }
```

```bash
export MT5_LOGIN=5057947437      # Windows PowerShell: $env:MT5_LOGIN="5057947437"
export MT5_PASSWORD="YourPassword"
export MT5_SERVER="MetaQuotes-Demo"
export MRPC_API_KEY="your_api_key"   # defaults to TRIAL if unset
```

Then launch the **interactive menu** and pick a demo by number, or run one directly:

```bash
./build/demos           # interactive menu (lowlevel / service / sugar / orchestrators / preset)
./build/demos 6         # run demo [6] (Sugar basics) directly
./build/protobuf_inspector   # offline: explore the gRPC messages, fields & enums
```

---

## 🔑 API key & authentication

Connecting to MetaRPC production (`mt5.mrpc.pro:443`) requires an API key:

1. **Sign up** — free at [https://mrpc.pro/signup](https://mrpc.pro/signup).
2. **Generate a key** — in the portal at [https://mrpc.pro/my](https://mrpc.pro/my), under **API Keys**.
3. **Provide it** — the SDK reads the `MRPC_API_KEY` environment variable (falling back to `TRIAL`) and sends it as the `apikey` metadata header on every call. You can also pass it to the layer constructor or set `api_key` in `config.json`.

### Automatic session id

Two things authenticate each request: your **`apikey`** and a terminal **session id**. You don't fetch the id — `connect()` obtains it from the gateway (`ConnectEx` → `terminalInstanceGuid`), and the SDK attaches it to every later call and stream automatically (read it via `terminalGuid()`).

---

## 🌐 Production endpoints

| Environment | Host | Port | Protocol |
| :--- | :--- | :--- | :--- |
| **Production** | `mt5.mrpc.pro` | `443` | TLS / gRPC |
| **Direct API UI (Swagger)** | `https://mt5.mrpc.pro/apiui` | `443` | HTTPS |
| **Portal Dashboard** | `https://mrpc.pro/my` | `443` | HTTPS |
| **Registration / API Key** | `https://mrpc.pro/signup` | `443` | HTTPS |

---

## 📄 Documentation & guides

Full docs at [https://metarpc.github.io/CppMT5/](https://metarpc.github.io/CppMT5/):

- 🚀 **[Getting Started](https://metarpc.github.io/CppMT5/All_Guides/GETTING_STARTED/)** & **[Your First Project](https://metarpc.github.io/CppMT5/All_Guides/Your_First_Project/)**
- 🧱 **[Project Map](https://metarpc.github.io/CppMT5/All_Guides/PROJECT_MAP/)** — the three layers and source layout
- 📡 **[gRPC Streaming](https://metarpc.github.io/CppMT5/All_Guides/GRPC_STREAM_MANAGEMENT/)** — live ticks, trades, position P/L
- 📊 **[Return Codes](https://metarpc.github.io/CppMT5/All_Guides/RETURN_CODES_REFERENCE/)** & **[ENUMs](https://metarpc.github.io/CppMT5/All_Guides/ENUMS_USAGE_REFERENCE/)** & **[Protobuf Inspector](https://metarpc.github.io/CppMT5/All_Guides/PROTOBUF_INSPECTOR_GUIDE/)**
- 💼 **API reference** — [MT5Account](https://metarpc.github.io/CppMT5/MT5Account/MT5Account.Master.Overview/) · [MT5Service](https://metarpc.github.io/CppMT5/MT5Service/MT5Service.Overview/) · [MT5Sugar](https://metarpc.github.io/CppMT5/MT5Sugar/MT5Sugar.API_Overview/)

---

## 📜 License

MIT
