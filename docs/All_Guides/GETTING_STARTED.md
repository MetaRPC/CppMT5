# Getting Started

CppMT5 is a C++17 client for the MetaRPC MT5 gRPC gateway. It ships as a small static library
with a three-layer API — [`MT5Account`](../API_Reference/MT5Account.md) (low-level protobuf),
[`MT5Service`](../API_Reference/MT5Service.md) (friendly facade) and
[`MT5Sugar`](../API_Reference/MT5Sugar.md) (one-liners).

## 1. Requirements

- A C++17 compiler
- CMake ≥ 3.15 and a generator (Ninja recommended)
- **protobuf** and **grpc++** development packages, discovered via `pkg-config`
- the **`protoc`** compiler and the **`grpc_cpp_plugin`** — the build generates the gRPC stubs from the shipped contract at configure time (the install commands below include them)

Install the dependencies:

=== "Debian / Ubuntu"
    ```bash
    sudo apt install build-essential cmake ninja-build pkg-config \
         libprotobuf-dev protobuf-compiler libgrpc++-dev protobuf-compiler-grpc
    ```

=== "MSYS2 (Windows, UCRT64)"
    ```bash
    pacman -S mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,pkgconf,protobuf,grpc}
    ```

=== "macOS (Homebrew)"
    ```bash
    brew install cmake ninja pkg-config protobuf grpc
    ```

!!! note "Generated protobuf code"
    The repository ships the gRPC contract as a single version-independent
    [`proto/mt5_term_api.desc`](https://github.com/MetaRPC/CppMT5/blob/main/proto/mt5_term_api.desc)
    (a protobuf `FileDescriptorSet`). At build time CMake runs **your** `protoc` on it, so the
    generated C++/gRPC stubs always match the protobuf/grpc installed above — no version pinning,
    nothing to regenerate by hand. If you only want the raw contract (to build your own client in
    any language), download that one file and feed it to `protoc --descriptor_set_in=…`.

## 2. Build

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

!!! info "What the first configure does"
    `git clone` + these two commands is all you need — **no manual `protoc` step.** The first
    `cmake` configure runs `protoc` on `proto/mt5_term_api.desc` to generate the C++/gRPC stubs into
    `build/generated/` (a few seconds), then compiles them into the library. That's why `protoc` and
    `grpc_cpp_plugin` must be installed (they're in the dependencies above). Later builds reuse the
    generated code; it regenerates only if the contract changes.

This produces the `metarpc_cppmt5` static library and the `demos` launcher (14 runnable
examples). Turn the demos off with `-DBUILD_DEMOS=OFF`.

## 3. Get an API key

Connecting to MetaRPC production (`mt5.mrpc.pro:443`) needs an API key:

1. **Sign up** — free at [https://mrpc.pro/signup](https://mrpc.pro/signup).
2. **Generate a key** — in the portal at [https://mrpc.pro/my](https://mrpc.pro/my), under **API Keys**.
3. **Provide it** — the SDK reads `MRPC_API_KEY` from the environment (falling back to `"TRIAL"`), and sends it as the `apikey` metadata header on every call. You can also pass it to the layer constructor or set `api_key` in `config.json`.

You also need an MT5 **demo account** (login, password, server name) — see [MT5 for Beginners](MT5_For_Beginners.md).

!!! note "The terminal session id is automatic"
    Two things authenticate each call: your **`apikey`** and a terminal **session id**. You don't
    fetch the id yourself — `connect()` obtains it from the gateway (`ConnectEx` →
    `terminalInstanceGuid`), the SDK stores it and attaches it to every later unary call and
    stream. Read it any time with `terminalGuid()`.

## 4. Configure credentials

The demos read `examples/demos/config/config.json` (or environment variables). Create it from
your MetaRPC account:

```json
{
  "user": 5057947437,
  "password": "YourPassword",
  "grpc_server": "mt5.mrpc.pro:443",
  "mt_cluster": "MetaQuotes-Demo",
  "api_key": "",
  "test_symbol": "EURUSD",
  "test_volume": 0.01
}
```

Environment variables override the file: `MT5_LOGIN`, `MT5_PASSWORD`, `MT5_SERVER`,
`MT5_GRPC`, `MT5_API_KEY`. The `api_key` is `""` for trial access or your gateway key for
production.

## 5. Run a demo

```bash
./build/demos        # interactive menu
./build/demos 6      # run demo [6] (Sugar basics) directly
```

## 6. Your first program

```cpp
#include <iostream>
#include <metarpc/mt5_sugar.hpp>

int main() {
    using namespace metarpc;
    MT5Sugar sugar("mt5.mrpc.pro", 443 /*, apiKey */);

    try {
        sugar.connect(5057947437, "YourPassword", "MetaQuotes-Demo");

        std::cout << "balance : " << sugar.getBalance() << "\n";
        std::cout << "EURUSD  : bid " << sugar.getBid("EURUSD")
                  << "  ask " << sugar.getAsk("EURUSD") << "\n";
    } catch (const ApiError& e) {
        std::cerr << "API error: " << e.code << ": " << e.message << "\n";
        return 1;
    } catch (const TransportError& e) {
        std::cerr << "transport error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
```

Link it against the library:

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE metarpc_cppmt5)
```

## Which layer should I use?

| You want… | Use |
|-----------|-----|
| The quickest possible code, smart defaults | [`MT5Sugar`](../API_Reference/MT5Sugar.md) |
| Friendly C++ structs, full method set, UTC times | [`MT5Service`](../API_Reference/MT5Service.md) |
| A field the facades don't expose, raw protobuf control | [`MT5Account`](../API_Reference/MT5Account.md) |

All three live in one object: a connected `MT5Sugar` exposes `service()` and `account()`, so you
can mix levels freely.

## Next steps

- [Your First Project](Your_First_Project.md) — a guided build.
- The `examples/demos/` folder — 14 runnable examples across all three layers, plus
  orchestrators and a preset.
