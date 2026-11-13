# 🗺️ CppMT5 Project Map

> The whole project as one annotated tree, with links. **Links point to section *overviews*, never to individual method pages** — open an overview to drill down into its methods.

---

## 🧱 The three layers

CppMT5 is built in three decoupled layers (composition, not inheritance). Each layer **owns** the one below it, so a connected `MT5Sugar` gives you all three. Your code can enter at any level.

| Layer | Source | What it does | Overview |
| ----- | ------ | ------------ | -------- |
| **3 · MT5Sugar** | `include/metarpc/mt5_sugar.hpp` · `src/mt5_sugar.cpp` | Convenience on top of Service: one-liners, pip-based SL/TP, risk-based sizing, batch close, quick reads. | [MT5Sugar overview](../MT5Sugar/MT5Sugar.API_Overview.md) |
| **2 · MT5Service** | `include/metarpc/mt5_service.hpp` · `src/mt5_service.cpp` | Friendly facade: plain args and named C++ structs, UTC times, **protobuf-free header**. What most code uses. | [MT5Service overview](../MT5Service/MT5Service.Overview.md) |
| **1 · MT5Account** | `include/metarpc/mt5_account.hpp` · `src/mt5_account.cpp` | Low-level gRPC: builds each proto `…Request`, attaches auth metadata, unwraps replies to `…Data` (or throws); connection & streaming. Everything builds on it. | [MT5Account overview](../MT5Account/MT5Account.Master.Overview.md) |

```text
Your code → MT5Sugar → MT5Service → MT5Account → gRPC → MetaRPC gateway → MT5 terminal
```

Mix freely: `sugar.service()` borrows the Service, `service.account()` (or `sugar.account()`) borrows the Account.

> **Compile-time note:** `mt5_service.hpp` and `mt5_sugar.hpp` are **protobuf-free** (they forward-declare `MT5Account`), so including them does not pull in the ~11 MB of generated headers. Only translation units that reach `mt5_account.hpp` — directly or via `account()` — pay that cost.

---

## 🌳 Repository tree

- **`CppMT5/`** — CMake project root
  - **`include/metarpc/`** — the public headers
    - `mt5_common.hpp` — `ApiError`, `TransportError`, `ClientOptions` (protobuf-free, shared)
    - `mt5_account.hpp` — **Layer 1** (pulls in protobuf) → [MT5Account overview](../MT5Account/MT5Account.Master.Overview.md)
    - `mt5_service.hpp` — **Layer 2** + friendly structs (protobuf-free) → [MT5Service overview](../MT5Service/MT5Service.Overview.md)
    - `mt5_sugar.hpp` — **Layer 3** (protobuf-free) → [MT5Sugar overview](../MT5Sugar/MT5Sugar.API_Overview.md)
    - `mt5.hpp` — umbrella; includes common + all three layers
  - **`src/`** — the library implementation
    - `mt5_account.cpp` — low-level gRPC machinery (channel, retry/reconnect, 1 method per RPC, streams)
    - `mt5_service.cpp` — protobuf → friendly conversion, UTC shifting
    - `mt5_sugar.cpp` — conveniences over MT5Service
  - **`proto/`** — the raw gRPC contract
    - `mt5_term_api.desc` — a version-independent `FileDescriptorSet` ([download on GitHub](https://github.com/MetaRPC/CppMT5/blob/main/proto/mt5_term_api.desc)). CMake runs `protoc` on it at build time to generate the C++/gRPC stubs (into `build/generated/`) matching your installed protobuf → [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md)
  - **`examples/`** — runnable code
    - `quickstart.cpp` — the smallest end-to-end program
    - **`demos/`** — the `demos` launcher (menu) + config/helpers, numbered by layer
      - **`lowlevel/`** (01–03) → [MT5Account overview](../MT5Account/MT5Account.Master.Overview.md)
      - **`service/`** (04–05) → [MT5Service overview](../MT5Service/MT5Service.Overview.md)
      - **`sugar/`** (06–10) → [MT5Sugar overview](../MT5Sugar/MT5Sugar.API_Overview.md)
      - **`orchestrators/`** (11, 13, 14) + **`presets/`** (16) → [Strategies overview](../Strategies/Strategies.Master.Overview.md)
      - `config/` (loads creds) · `helpers/connection.hpp` · `main.cpp` (menu + dispatch)
  - `CMakeLists.txt` — build (library + `demos` target; pkg-config for grpc++/protobuf)
  - `mkdocs.yml` — docs site config
  - **`docs/`** — this documentation

---

## 📚 Documentation tree

- **`docs/`**
  - [Home](../index.md) — landing page
  - **`All_Guides/`** — the guides: [Getting Started](GETTING_STARTED.md), [Your First Project](Your_First_Project.md), [MT5 for Beginners](MT5_For_Beginners.md), [Glossary](GLOSSARY.md), [Concurrency Model](SYNC_VS_ASYNC.md), [gRPC Streaming](GRPC_STREAM_MANAGEMENT.md), [Enums](ENUMS_USAGE_REFERENCE.md), [Return Codes](RETURN_CODES_REFERENCE.md), [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md)
  - **`MT5Account/`** — low-level gRPC docs → [overview](../MT5Account/MT5Account.Master.Overview.md) *(per-method pages inside, in 7 group folders)*
  - **`MT5Service/`** — friendly facade docs → [overview](../MT5Service/MT5Service.Overview.md) *(per-group pages inside)*
  - **`MT5Sugar/`** — convenience API docs → [overview](../MT5Sugar/MT5Sugar.API_Overview.md) *(per-group pages inside)*
  - **`Strategies/`** — orchestrators + presets → [overview](../Strategies/Strategies.Master.Overview.md) *(per-strategy pages inside)*
  - `API_Reference/` — consolidated per-layer reference pages
  - `styles/` · `javascripts/` · `includes/` — assets & theme

> The `MT5Account/`, `MT5Service/`, `MT5Sugar/` and `Strategies/` folders hold per-method / per-group / per-strategy pages. By design this map links only their **overviews** — each overview is the index into its own detail pages.

---

## 🎯 Which layer should you use?

- **Most trading apps → [MT5Sugar](../MT5Sugar/MT5Sugar.API_Overview.md).** Risk sizing, pip math and bulk ops out of the box.
- **Custom execution → [MT5Service](../MT5Service/MT5Service.Overview.md).** Friendly typed calls when you manage your own order shapes.
- **Protocol-level control → [MT5Account](../MT5Account/MT5Account.Master.Overview.md).** Raw proto structures and streams.

---

## 🔧 Transport behaviour (shared by every call)

- **Retry** — transient gRPC failures (`UNAVAILABLE`, `DEADLINE_EXCEEDED`) are retried with exponential back-off, up to `ClientOptions::maxRetries`.
- **Reconnect** — a `TERMINAL_INSTANCE_NOT_FOUND` transparently re-issues `ConnectEx` and retries, so a terminal restart is invisible to your code.
- **Errors** — anything else becomes an `ApiError` (server said no) or `TransportError` (transport gave up). See [Return Codes](RETURN_CODES_REFERENCE.md).

---

## 📚 See also

- [Getting Started](GETTING_STARTED.md) · [Glossary](GLOSSARY.md) · [Protobuf Inspector](PROTOBUF_INSPECTOR_GUIDE.md)
