# Protobuf Inspector — Interactive Type Explorer

> An offline developer tool that explores the MT5 gRPC protobuf **messages, fields and enums** — browse, search, find which message declares a field, and list enum values, all **without connecting to MT5**.

**Source:** `examples/tools/protobuf_inspector.cpp` · **Run:** `./build/protobuf_inspector`

---

## How C++ does this

The C# / Go / Java inspectors use *runtime reflection* over the protobuf descriptors their libraries keep in memory — and **so does this one.** Unlike Rust's `prost` (which keeps no runtime descriptors), the C++ protobuf runtime registers a full descriptor for every generated message and enum at static-init time.

So this tool simply reads the **global generated descriptor pool**:

```cpp
const auto* pool = google::protobuf::DescriptorPool::generated_pool();
```

and walks the `mt5_term_api` proto files compiled into the library. Because it reads the very descriptors the SDK uses on the wire, it can never drift from your types — the field names, tags, proto types and `repeated` / `map` / `oneof` shapes it prints are exactly what the generated C++ structs expose.

!!! note "Why `--whole-archive`"
    The generated `.pb.cc` live in the static library `metarpc_cppmt5`. A static library only pulls
    in a translation unit that something references — so descriptors for messages your `main()`
    never names would be dropped, and the inspector would see an incomplete pool. The CMake target
    links the library with `-Wl,--whole-archive` (MSVC: `/WHOLEARCHIVE`) so **every** generated file
    registers itself. That's the whole trick.

---

## 🏁 Running it

It's a standalone target and needs no credentials or connection:

```bash
cmake --build build --target protobuf_inspector
./build/protobuf_inspector
```

```
╔══════════════════════════════════════════════════════╗
║   🔍 PROTOBUF INSPECTOR — mt5_term_api               ║
║  Explore MT5 gRPC messages & enums offline           ║
╚══════════════════════════════════════════════════════╝
Indexed 359 messages, 70 enums from the generated code.
Type 'help' for commands.

>
```

---

## 🧰 Commands

| Command | Description | Example |
|---------|-------------|---------|
| `list` / `ls` | List all message types | `list` |
| `enums` | List all enum types | `enums` |
| `<TypeName>` | Inspect a message — its fields and types | `AccountSummaryData` |
| `search <text>` / `find <text>` | Search types by name | `search Position` |
| `field <name>` | Find messages that declare a field | `field ticket` |
| `enum <name>` | Show an enum's values | `enum TMT5_ENUM_ORDER_TYPE` |
| `help` / `?` | Help | `help` |
| `exit` / `quit` / `q` | Exit | `exit` |

Type names and field names are **case-insensitive**. A bare token is resolved as a message first, then an enum.

---

## Practical examples

### 1) Inspect a message's fields

```
> AccountSummaryData
```
```
╔═══════════════════════ MESSAGE: AccountSummaryData ═══════════════════════╗

📋 Fields (21):

  • #1   account_login                             : int64
  • #2   account_balance                           : double
  • #3   account_equity                            : double
  • #5   account_leverage                          : int64
  • #6   account_trade_mode                        : MrpcEnumAccountTradeMode
  • #9   server_time                               : Timestamp
  • #14  server_time_utc                           : Timestamp
  • #17  account_margin_mode                       : MrpcEnumAccountMarginMode
  ...
```

`#n` is the protobuf field tag; the type is the proto type. `repeated …` and `map<…>` are shown where relevant, and message / enum fields show the referenced type name. These are the real accessors on the generated class — `summary.account_balance()`, `summary.account_trade_mode()`, etc. (snake_case getters in C++).

### 2) A `oneof` reply

```
> AccountSummaryReply
```
```
╔═════════════ MESSAGE: AccountSummaryReply ═════════════╗

📋 Fields (2):

  • #1   data    : AccountSummaryData   (oneof response)
  • #2   error   : Error                (oneof response)
```

The `*Reply` messages wrap their payload in a `response` oneof of `Data` | `Error` — exactly what `MT5Account` unwraps (returning the `Data`, or throwing `ApiError` for the `Error`).

### 3) Which messages carry a `ticket` field?

```
> field ticket
```
```
  📦 DealHistoryData
       └─ ticket : uint64
  📦 OpenedOrderInfo
       └─ ticket : uint64
  📦 OrderCloseRequest
       └─ ticket : uint64
  📦 OrderModifyRequest
       └─ ticket : uint64
  📦 PositionInfo
       └─ ticket : uint64
  📦 OnPositionProfitPositionInfo
       └─ ticket : int64
  ...
```

### 4) See an enum's values

```
> enum TMT5_ENUM_ORDER_TYPE
```
```
╔═════════════ ENUM: TMT5_ENUM_ORDER_TYPE ═════════════╗

  TMT5_ORDER_TYPE_BUY                            = 0
  TMT5_ORDER_TYPE_SELL                           = 1
  TMT5_ORDER_TYPE_BUY_LIMIT                      = 2
  TMT5_ORDER_TYPE_SELL_LIMIT                     = 3
  TMT5_ORDER_TYPE_BUY_STOP                       = 4
  TMT5_ORDER_TYPE_SELL_STOP                      = 5
  TMT5_ORDER_TYPE_BUY_STOP_LIMIT                 = 6
  TMT5_ORDER_TYPE_SELL_STOP_LIMIT                = 7
  TMT5_ORDER_TYPE_CLOSE_BY                       = 8
```

> In C++ these enum values live at **namespace scope** — you write `pb::TMT5_ORDER_TYPE_BUY` (where `namespace pb = mt5_term_api;`), not a prefixed form. See [Enums Reference](ENUMS_USAGE_REFERENCE.md).

### 5) Search types by name

```
> search Position
```
```
  📦 mt5_term_api.PositionInfo
  📦 mt5_term_api.PositionHistoryInfo
  📦 mt5_term_api.PositionsHistoryData
  📦 mt5_term_api.PositionsTotalData
  🔤 mt5_term_api.BMT5_ENUM_POSITION_TYPE (enum)
  🔤 mt5_term_api.AH_ENUM_POSITIONS_HISTORY_SORT_TYPE (enum)
  ...
```

---

## 🧩 How it works

- `DescriptorPool::generated_pool()` returns the global pool the protobuf runtime fills at static-init.
- The tool calls `FindFileByName(...)` for each of the **19 `mt5-term-api-*.proto`** files, then walks `file->message_type(i)` / `file->enum_type(i)`, recursing into nested types — building the complete **359 messages / 70 enums** index.
- For each message it reads `Descriptor::field(i)`: the tag (`field->number()`), name (`field->name()`), proto type (`field->type_name()`, or the referenced `message_type()` / `enum_type()` name), and shape (`is_repeated()`, `is_map()`, `real_containing_oneof()`).
- Everything is **static** — no terminal connection, no network, no extra dependencies beyond the protobuf runtime the SDK already links.

Source: `examples/tools/protobuf_inspector.cpp`.

> **Want just the contract?** The whole gRPC contract everything is built on ships as one
> version-independent file,
> [`proto/mt5_term_api.desc`](https://github.com/MetaRPC/CppMT5/blob/main/proto/mt5_term_api.desc)
> (a protobuf `FileDescriptorSet`). Download that single file and generate stubs for any language:
> `protoc --descriptor_set_in=mt5_term_api.desc --python_out=. <file>.proto`. CMake does exactly
> this (for C++) at build time, so the SDK never ships version-locked generated code.

---

**See also:** [Enums Reference](ENUMS_USAGE_REFERENCE.md) · [Project Map](PROJECT_MAP.md) · [Glossary](GLOSSARY.md) · [Return Codes](RETURN_CODES_REFERENCE.md)
