// MetaRPC CppMT5 — real gRPC implementation over the generated stubs in package/.
#include "metarpc/mt5.hpp"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/empty.pb.h>

#include "mt5-term-api-connection.grpc.pb.h"
#include "mt5-term-api-account-helper.grpc.pb.h"
#include "mt5-term-api-account-information.grpc.pb.h"
#include "mt5-term-api-market-info.grpc.pb.h"
#include "mt5-term-api-subscriptions.grpc.pb.h"
#include "mt5-term-api-trading-helper.grpc.pb.h"
#include "mt5-term-api-trade-functions.grpc.pb.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>

namespace pb = mt5_term_api;

namespace metarpc {

// ======================================================================================
// Hashing helpers (SHA-256 for the legacy id, SHA-1 for RFC 4122 uuid5)
// ======================================================================================
namespace {

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t rotl(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

std::vector<uint8_t> sha256Bytes(const std::string& input) {
    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;
    std::vector<uint8_t> msg(input.begin(), input.end());
    uint64_t bit_len = (uint64_t)msg.size() * 8;
    msg.push_back(0x80);
    while ((msg.size() + 8) % 64 != 0) msg.push_back(0x00);
    for (int i = 7; i >= 0; --i) msg.push_back((uint8_t)((bit_len >> (i * 8)) & 0xFF));
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = ((uint32_t)msg[chunk + i * 4] << 24) | ((uint32_t)msg[chunk + i * 4 + 1] << 16) |
                   ((uint32_t)msg[chunk + i * 4 + 2] << 8) | ((uint32_t)msg[chunk + i * 4 + 3]);
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = h + S1 + ch + K256[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e; h5 += f; h6 += g; h7 += h;
    }
    std::vector<uint8_t> hash(32);
    uint32_t hh[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (int i = 0; i < 8; ++i) {
        hash[i * 4] = (uint8_t)(hh[i] >> 24); hash[i * 4 + 1] = (uint8_t)(hh[i] >> 16);
        hash[i * 4 + 2] = (uint8_t)(hh[i] >> 8); hash[i * 4 + 3] = (uint8_t)(hh[i]);
    }
    return hash;
}

std::vector<uint8_t> sha1Bytes(const std::vector<uint8_t>& input) {
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    std::vector<uint8_t> msg(input);
    uint64_t bit_len = (uint64_t)input.size() * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0x00);
    for (int i = 7; i >= 0; --i) msg.push_back((uint8_t)((bit_len >> (i * 8)) & 0xFF));
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = ((uint32_t)msg[chunk + i * 4] << 24) | ((uint32_t)msg[chunk + i * 4 + 1] << 16) |
                   ((uint32_t)msg[chunk + i * 4 + 2] << 8) | ((uint32_t)msg[chunk + i * 4 + 3]);
        for (int i = 16; i < 80; ++i) w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            uint32_t t = rotl(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rotl(b, 30); b = a; a = t;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    std::vector<uint8_t> out(20);
    uint32_t hh[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        out[i * 4] = (uint8_t)(hh[i] >> 24); out[i * 4 + 1] = (uint8_t)(hh[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(hh[i] >> 8); out[i * 4 + 3] = (uint8_t)(hh[i]);
    }
    return out;
}

std::string formatUuid(const uint8_t* b) {
    char buf[37];
    snprintf(buf, sizeof(buf),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    return std::string(buf);
}

std::vector<uint8_t> parseUuid(const std::string& s) {
    std::vector<uint8_t> out;
    std::string hex;
    for (char c : s) if (c != '-' && c != '{' && c != '}') hex.push_back(c);
    if (hex.size() != 32) throw std::invalid_argument("uuid5: namespace is not a UUID: " + s);
    for (size_t i = 0; i < 32; i += 2) out.push_back((uint8_t)std::stoul(hex.substr(i, 2), nullptr, 16));
    return out;
}

int64_t nowUnix() {
    return (int64_t)std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string readFileIfExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

ApiError makeApiError(const pb::Error& e, const std::string& where) {
    std::string msg = e.error_message();
    if (!e.mql_error_description().empty()) msg += " [mql: " + e.mql_error_description() + "]";
    if (!e.mql_error_trade_description().empty()) msg += " [trade: " + e.mql_error_trade_description() + "]";
    for (int i = 0; i < e.properties_size(); ++i)
        msg += " {" + e.properties(i).error_arg_key() + "=" + e.properties(i).error_arg_value() + "}";
    return ApiError(e.error_code(), msg, e.mql_error_int_code(),
                    e.mql_error_trade_int_code(), e.mql_error_trade_description(), where);
}

template <class Reply>
bool replyHasError(const Reply& r) {
    return r.has_error() && (!r.error().error_code().empty() || !r.error().error_message().empty());
}

Tick tickFrom(const pb::MrpcMqlTick& t, const std::string& symbol) {
    Tick o; o.symbol = symbol; o.time = t.time(); o.timeMsc = t.time_msc(); o.bid = t.bid(); o.ask = t.ask();
    o.last = t.last(); o.volume = t.volume_real() > 0 ? t.volume_real() : (double)t.volume(); o.flags = t.flags();
    if (o.time == 0 && o.timeMsc > 0) o.time = o.timeMsc / 1000;
    return o;
}

Tick tickFrom(const pb::MrpcSubscriptionMqlTick& t) {
    Tick o; o.symbol = t.symbol(); o.time = t.time().seconds(); o.timeMsc = t.time_msc(); o.bid = t.bid(); o.ask = t.ask();
    o.last = t.last(); o.volume = t.volume_real() > 0 ? t.volume_real() : (double)t.volume(); o.flags = t.flags();
    if (o.time == 0 && o.timeMsc > 0) o.time = o.timeMsc / 1000;
    return o;
}

SymbolInfo symbolFrom(const pb::SymbolParameters& p) {
    SymbolInfo s;
    s.name = p.name(); s.description = p.sym_description(); s.path = p.path();
    s.currencyBase = p.currency_base(); s.currencyProfit = p.currency_profit(); s.currencyMargin = p.currency_margin();
    s.digits = p.digits(); s.point = p.point(); s.bid = p.bid(); s.ask = p.ask(); s.spread = p.spread();
    s.contractSize = p.trade_contract_size(); s.tickSize = p.trade_tick_size(); s.tickValue = p.trade_tick_value();
    s.volumeMin = p.volume_min(); s.volumeMax = p.volume_max(); s.volumeStep = p.volume_step(); s.volumeLimit = p.volume_limit();
    s.stopsLevel = p.trade_stops_level(); s.freezeLevel = p.trade_freeze_level();
    s.tradeMode = (int)p.trade_mode(); s.tradeCalcMode = (int)p.trade_calc_mode(); s.tradeExeMode = (int)p.trade_exe_mode();
    s.swapMode = (int)p.swap_mode(); s.swapLong = p.swap_long(); s.swapShort = p.swap_short();
    for (int i = 0; i < p.filling_mode_size(); ++i) s.fillingModes.push_back((int)p.filling_mode(i));
    s.visible = p.visible(); s.selected = p.select(); s.exist = p.exist(); s.time = p.time().seconds();
    return s;
}

Position positionFrom(const pb::PositionInfo& p) {
    Position o; o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.type(); o.volume = p.volume();
    o.priceOpen = p.price_open(); o.priceCurrent = p.price_current(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit();
    o.profit = p.profit(); o.swap = p.swap(); o.commission = p.position_commission(); o.magic = p.magic_number();
    o.comment = p.comment(); o.openTime = p.open_time().seconds(); o.identifier = p.identifier(); o.reason = (int)p.reason();
    return o;
}

PendingOrder pendingFrom(const pb::OpenedOrderInfo& p) {
    PendingOrder o; o.ticket = (int64_t)p.ticket(); o.symbol = p.symbol(); o.type = (int)p.type(); o.state = (int)p.state();
    o.volumeInitial = p.volume_initial(); o.volumeCurrent = p.volume_current(); o.priceOpen = p.price_open();
    o.priceCurrent = p.price_current(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit(); o.stopLimit = p.stop_limit();
    o.magic = p.magic_number(); o.comment = p.comment(); o.timeSetup = p.time_setup().seconds();
    o.timeExpiration = p.time_expiration().seconds(); o.positionId = p.position_id();
    return o;
}

Deal dealFrom(const pb::DealHistoryData& d) {
    Deal o; o.ticket = (int64_t)d.ticket(); o.symbol = d.symbol(); o.type = (int)d.type(); o.entry = (int)d.entry_type();
    o.volume = d.volume(); o.price = d.price(); o.profit = d.profit(); o.commission = d.commission(); o.swap = d.swap();
    o.fee = d.fee(); o.stopLoss = d.stop_loss(); o.takeProfit = d.take_profit(); o.positionId = (int64_t)d.position_id();
    o.time = d.time().seconds(); o.comment = d.comment(); o.reason = (int)d.reason();
    return o;
}

HistoryOrder historyOrderFrom(const pb::OrderHistoryData& h) {
    HistoryOrder o; o.ticket = (int64_t)h.ticket(); o.symbol = h.symbol(); o.type = (int)h.type(); o.state = (int)h.state();
    o.priceOpen = h.price_open(); o.priceCurrent = h.price_current(); o.stopLoss = h.stop_loss(); o.takeProfit = h.take_profit();
    o.volumeInitial = h.volume_initial(); o.volumeCurrent = h.volume_current(); o.magic = h.magic_number(); o.comment = h.comment();
    o.setupTime = h.setup_time().seconds(); o.doneTime = h.done_time().seconds(); o.positionId = (int64_t)h.position_id();
    return o;
}

template <class Data>
OrderResult orderResultFrom(const Data& d) {
    OrderResult r; r.retcode = d.returned_code(); r.deal = (int64_t)d.deal(); r.order = (int64_t)d.order(); r.ticket = r.order;
    r.volume = d.volume(); r.price = d.price(); r.bid = d.bid(); r.ask = d.ask(); r.comment = d.comment();
    r.retcodeName = d.returned_string_code(); r.retcodeDescription = d.returned_code_description(); r.requestId = d.request_id();
    return r;
}

} // namespace

int timeframeSeconds(Timeframe tf) {
    int v = (int)tf;
    if (v < 16385) return v * 60;
    if (v < 32769) return (v - 16384) * 3600;
    if (v == 32769) return 7 * 86400;
    return 30 * 86400;
}

const char* timeframeName(Timeframe tf) {
    switch (tf) {
        case Timeframe::M1: return "M1"; case Timeframe::M2: return "M2"; case Timeframe::M3: return "M3";
        case Timeframe::M4: return "M4"; case Timeframe::M5: return "M5"; case Timeframe::M6: return "M6";
        case Timeframe::M10: return "M10"; case Timeframe::M12: return "M12"; case Timeframe::M15: return "M15";
        case Timeframe::M20: return "M20"; case Timeframe::M30: return "M30"; case Timeframe::H1: return "H1";
        case Timeframe::H2: return "H2"; case Timeframe::H3: return "H3"; case Timeframe::H4: return "H4";
        case Timeframe::H6: return "H6"; case Timeframe::H8: return "H8"; case Timeframe::H12: return "H12";
        case Timeframe::D1: return "D1"; case Timeframe::W1: return "W1"; case Timeframe::MN1: return "MN1";
    }
    return "?";
}

// ======================================================================================
// Static id helpers
// ======================================================================================
std::string MT5Client::uuid5(const std::string& namespaceUuid, const std::string& name) {
    std::vector<uint8_t> data = parseUuid(namespaceUuid);
    data.insert(data.end(), name.begin(), name.end());
    std::vector<uint8_t> h = sha1Bytes(data);
    h[6] = (uint8_t)((h[6] & 0x0F) | 0x50);
    h[8] = (uint8_t)((h[8] & 0x3F) | 0x80);
    return formatUuid(h.data());
}

std::string MT5Client::computeDeterministicId(int64_t login, const std::string& password) {
    std::string input = std::to_string(login) + ":" + password;
    std::vector<uint8_t> h = sha256Bytes(input);
    uint8_t le[16];
    le[0] = h[3]; le[1] = h[2]; le[2] = h[1]; le[3] = h[0];
    le[4] = h[5]; le[5] = h[4]; le[6] = h[7]; le[7] = h[6];
    for (int i = 8; i < 16; ++i) le[i] = h[i];
    return formatUuid(le);
}

// ======================================================================================
// TickStream
// ======================================================================================
struct TickStream::Impl {
    grpc::ClientContext ctx;
    std::unique_ptr<grpc::ClientReader<pb::OnSymbolTickReply>> reader;
    std::thread th;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> count{0};
    mutable std::mutex mu;
    std::string err;
    std::function<void(const Tick&)> cb;
};

TickStream::TickStream() : m_impl(new Impl()) {}
TickStream::~TickStream() { stop(); }

void TickStream::stop() {
    if (!m_impl) return;
    if (m_impl->th.joinable()) {
        m_impl->ctx.TryCancel();
        m_impl->th.join();
    }
    m_impl->running = false;
}
bool TickStream::running() const { return m_impl && m_impl->running.load(); }
uint64_t TickStream::count() const { return m_impl ? m_impl->count.load() : 0; }
std::string TickStream::lastError() const {
    if (!m_impl) return "";
    std::lock_guard<std::mutex> lk(m_impl->mu);
    return m_impl->err;
}

// ======================================================================================
// MT5Client::Impl
// ======================================================================================
struct MT5Client::Impl {
    std::string host;
    int port;
    std::string apiKey;
    std::string name;
    std::string id;             // header "id"
    std::string terminalGuid;   // what ConnectEx returned
    int64_t login{0};
    std::string password;
    std::string server;
    std::string baseChartSymbol{"EURUSD"};
    int connectTimeoutSeconds{120};
    std::atomic<bool> connected{false};
    ClientOptions opts;
    std::mutex mu;              // guards id/terminalGuid/connected transitions

    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<pb::Connection::Stub> conn;
    std::unique_ptr<pb::AccountHelper::Stub> helper;
    std::unique_ptr<pb::AccountInformation::Stub> accInfo;
    std::unique_ptr<pb::MarketInfo::Stub> market;
    std::unique_ptr<pb::SubscriptionService::Stub> subs;
    std::unique_ptr<pb::TradingHelper::Stub> trade;
    std::unique_ptr<pb::TradeFunctions::Stub> tradeFn;

    std::mt19937 rng{std::random_device{}()};

    // Broker server clock offset from UTC (seconds). Learned from AccountSummary and cached.
    std::atomic<int64_t> offsetSec{0};
    std::atomic<bool> offsetKnown{false};

    void log(const std::string& s) {
        if (opts.verbose) std::cerr << "[cppmt5] " << s << std::endl;
    }

    int64_t offset();                                   // defined after call<> (needs it)
    int64_t in(int64_t utc) { return (utc != 0 && opts.utcTimes) ? utc + offset() : utc; }      // UTC -> server
    int64_t out(int64_t srv) { return (srv != 0 && opts.utcTimes) ? srv - offset() : srv; }     // server -> UTC
    int64_t outMs(int64_t srvMs) { return (srvMs != 0 && opts.utcTimes) ? srvMs - offset() * 1000 : srvMs; }

    void ensureChannel() {
        if (channel) return;
        grpc::SslCredentialsOptions ssl;
        std::string pem;
        if (!opts.rootCertsPath.empty()) pem = readFileIfExists(opts.rootCertsPath);
        if (pem.empty() && std::getenv("GRPC_DEFAULT_SSL_ROOTS_FILE_PATH") == nullptr) {
            const char* candidates[] = {"/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt",
                                        "/usr/share/grpc/roots.pem", "/etc/ssl/cert.pem"};
            for (const char* c : candidates) { pem = readFileIfExists(c); if (!pem.empty()) break; }
        }
        if (!pem.empty()) ssl.pem_root_certs = pem;
        grpc::ChannelArguments args;
        args.SetMaxReceiveMessageSize(opts.maxReceiveMessageBytes);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        std::string target = host + ":" + std::to_string(port);
        std::shared_ptr<grpc::ChannelCredentials> creds =
            (port == 443 || port == 8443) ? grpc::SslCredentials(ssl) : grpc::InsecureChannelCredentials();
        channel = grpc::CreateCustomChannel(target, creds, args);
        conn = pb::Connection::NewStub(channel);
        helper = pb::AccountHelper::NewStub(channel);
        accInfo = pb::AccountInformation::NewStub(channel);
        market = pb::MarketInfo::NewStub(channel);
        subs = pb::SubscriptionService::NewStub(channel);
        trade = pb::TradingHelper::NewStub(channel);
        tradeFn = pb::TradeFunctions::NewStub(channel);
    }

    void fillContext(grpc::ClientContext& ctx, int timeoutMs) {
        std::string curId;
        { std::lock_guard<std::mutex> lk(mu); curId = id; }
        if (!curId.empty()) ctx.AddMetadata("id", curId);
        ctx.AddMetadata("apikey", apiKey.empty() ? std::string("TRIAL") : apiKey);
        if (timeoutMs > 0)
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMs));
    }

    void sleepBackoff(int& delayMs) {
        std::uniform_int_distribution<int> jitter(-delayMs / 4, delayMs / 4);
        int wait = delayMs + jitter(rng);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait));
        delayMs = std::min(delayMs * 2, opts.maxBackoffMs);
    }

    // The core pattern: transport retry + TERMINAL_INSTANCE_NOT_FOUND reconnect + API error mapping.
    template <class Reply, class Fn>
    Reply call(const char* what, Fn&& fn, int timeoutMs, bool allowReconnect) {
        ensureChannel();
        int attempt = 0;
        int delayMs = 500;
        for (;;) {
            grpc::ClientContext ctx;
            fillContext(ctx, timeoutMs);
            Reply reply;
            grpc::Status st = fn(ctx, reply);
            if (!st.ok()) {
                auto c = st.error_code();
                bool transient = (c == grpc::StatusCode::UNAVAILABLE || c == grpc::StatusCode::DEADLINE_EXCEEDED);
                if (transient && attempt < opts.maxRetries) {
                    log(std::string(what) + ": grpc " + std::to_string((int)c) + " " + st.error_message() +
                        " -> retry in " + std::to_string(delayMs) + "ms");
                    ++attempt;
                    sleepBackoff(delayMs);
                    continue;
                }
                throw TransportError((int)c, std::string(what) + ": grpc status " + std::to_string((int)c) + ": " + st.error_message());
            }
            if (replyHasError(reply)) {
                const pb::Error& e = reply.error();
                if (allowReconnect && attempt < opts.maxRetries &&
                    (e.error_code() == "TERMINAL_INSTANCE_NOT_FOUND" || e.error_code() == "TERMINAL_REGISTRY_TERMINAL_NOT_FOUND")) {
                    log(std::string(what) + ": " + e.error_code() + " -> reconnect");
                    ++attempt;
                    connectEx();
                    sleepBackoff(delayMs);
                    continue;
                }
                throw makeApiError(e, what);
            }
            return reply;
        }
    }

    void connectEx() {
        if (login == 0 || server.empty()) throw ApiError("NOT_CONFIGURED", "connect() was never called", 0, 0, "", "ConnectEx");
        pb::ConnectExRequest req;
        req.set_user((uint64_t)login);
        req.set_password(password);
        req.set_mt_cluster_name(server);
        if (!baseChartSymbol.empty()) req.set_base_chart_symbol(baseChartSymbol);
        req.set_timeout_seconds((uint32_t)connectTimeoutSeconds);
        if (!name.empty()) req.set_name(name);
        auto reply = call<pb::ConnectExReply>("ConnectEx",
            [&](grpc::ClientContext& ctx, pb::ConnectExReply& r) { return conn->ConnectEx(&ctx, req, &r); },
            opts.connectTimeoutMs, /*allowReconnect*/ false);
        std::lock_guard<std::mutex> lk(mu);
        terminalGuid = reply.data().terminal_instance_guid();
        if (!terminalGuid.empty() && terminalGuid != id) {
            log("gateway returned terminal id " + terminalGuid + " (requested " + id + ") - adopting it");
            id = terminalGuid;
        }
        connected = true;
    }
};

int64_t MT5Client::Impl::offset() {
    if (offsetKnown.load()) return offsetSec.load();
    pb::AccountSummaryRequest req;
    auto reply = call<pb::AccountSummaryReply>("AccountSummary(offset)",
        [&](grpc::ClientContext& ctx, pb::AccountSummaryReply& r) { return helper->AccountSummary(&ctx, req, &r); },
        opts.unaryTimeoutMs, true);
    offsetSec = reply.data().utc_timezone_server_time_shift_minutes() * 60;
    offsetKnown = true;
    log("server time offset " + std::to_string(offsetSec.load()) + " s");
    return offsetSec.load();
}

// ======================================================================================
// MT5Client
// ======================================================================================
int64_t MT5Client::serverOffsetSeconds() { return m_impl->offset(); }
int64_t MT5Client::toUtc(int64_t serverTime) { return serverTime ? serverTime - m_impl->offset() : 0; }
int64_t MT5Client::toServerTime(int64_t utc) { return utc ? utc + m_impl->offset() : 0; }

MT5Client::MT5Client(const std::string& host, int port, const std::string& apiKey, const std::string& name)
    : m_impl(new Impl()) {
    m_impl->host = host;
    m_impl->port = port;
    m_impl->apiKey = apiKey;
    m_impl->name = name;
    if (m_impl->apiKey.empty()) {
        const char* envKey = std::getenv("MRPC_API_KEY");
        if (envKey) m_impl->apiKey = envKey;
    }
    const char* v = std::getenv("MRPC_VERBOSE");
    if (v && *v && std::strcmp(v, "0") != 0) m_impl->opts.verbose = true;
}

// NOTE: the destructor does NOT send Disconnect. The gateway keeps the terminal alive for
// the stable id, which is exactly what a 24/7 strategy wants across restarts.
MT5Client::~MT5Client() { close(); }

std::string MT5Client::getId(int64_t login, const std::string& password) {
    std::lock_guard<std::mutex> lk(m_impl->mu);
    m_impl->id = computeDeterministicId(login, password);
    return m_impl->id;
}
void MT5Client::setId(const std::string& id) { std::lock_guard<std::mutex> lk(m_impl->mu); m_impl->id = id; }
const std::string& MT5Client::getId() const { return m_impl->id; }
void MT5Client::setApiKey(const std::string& apiKey) { m_impl->apiKey = apiKey; }
const std::string& MT5Client::getApiKey() const { return m_impl->apiKey; }
void MT5Client::setName(const std::string& name) { m_impl->name = name; }
const std::string& MT5Client::getName() const { return m_impl->name; }
ClientOptions& MT5Client::options() { return m_impl->opts; }
std::string MT5Client::terminalGuid() const { std::lock_guard<std::mutex> lk(m_impl->mu); return m_impl->terminalGuid; }
bool MT5Client::isConnected() const { return m_impl->connected.load(); }

void MT5Client::connect(int64_t login, const std::string& password, const std::string& server,
                        const std::string& name, const std::string& baseChartSymbol, int timeoutSeconds) {
    m_impl->login = login;
    m_impl->password = password;
    m_impl->server = server;
    m_impl->baseChartSymbol = baseChartSymbol;
    m_impl->connectTimeoutSeconds = timeoutSeconds;
    if (!name.empty()) m_impl->name = name;
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        if (m_impl->id.empty()) m_impl->id = computeDeterministicId(login, password);
    }
    m_impl->ensureChannel();
    m_impl->connectEx();
}

void MT5Client::reconnect() { m_impl->connectEx(); }

void MT5Client::disconnect(const std::string& reason) {
    if (!m_impl->channel || m_impl->id.empty()) { m_impl->connected = false; return; }
    pb::DisconnectRequest req;
    if (!reason.empty()) req.set_reason(reason);
    try {
        m_impl->call<pb::DisconnectReply>("Disconnect",
            [&](grpc::ClientContext& ctx, pb::DisconnectReply& r) { return m_impl->conn->Disconnect(&ctx, req, &r); },
            10000, false);
    } catch (...) {
        m_impl->connected = false;
        throw;
    }
    m_impl->connected = false;
}

void MT5Client::close() {
    m_impl->connected = false;
    m_impl->conn.reset(); m_impl->helper.reset(); m_impl->accInfo.reset(); m_impl->market.reset();
    m_impl->subs.reset(); m_impl->trade.reset(); m_impl->tradeFn.reset();
    m_impl->channel.reset();
}

// --- account --------------------------------------------------------------------------
AccountInfo MT5Client::accountSummary() {
    pb::AccountSummaryRequest req;
    auto reply = m_impl->call<pb::AccountSummaryReply>("AccountSummary",
        [&](grpc::ClientContext& ctx, pb::AccountSummaryReply& r) { return m_impl->helper->AccountSummary(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    const auto& d = reply.data();
    AccountInfo a;
    a.login = d.account_login(); a.currency = d.account_currency(); a.balance = d.account_balance();
    a.equity = d.account_equity(); a.credit = d.account_credit(); a.leverage = d.account_leverage();
    a.name = d.account_user_name(); a.company = d.account_company_name();
    a.tradeMode = (TradeMode)(int)d.account_trade_mode();
    a.serverTime = d.server_time().seconds(); a.utcShiftMinutes = d.utc_timezone_server_time_shift_minutes();
    m_impl->offsetSec = a.utcShiftMinutes * 60;
    m_impl->offsetKnown = true;
    a.serverTimeUtc = a.serverTime ? a.serverTime - a.utcShiftMinutes * 60 : 0;
    a.server = m_impl->server;
    return a;
}

double MT5Client::accountInfoDouble(int propertyId) {
    pb::AccountInfoDoubleRequest req; req.set_property_id((pb::AccountInfoDoublePropertyType)propertyId);
    auto reply = m_impl->call<pb::AccountInfoDoubleReply>("AccountInfoDouble",
        [&](grpc::ClientContext& ctx, pb::AccountInfoDoubleReply& r) { return m_impl->accInfo->AccountInfoDouble(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().requested_value();
}

int64_t MT5Client::accountInfoInteger(int propertyId) {
    pb::AccountInfoIntegerRequest req; req.set_property_id((pb::AccountInfoIntegerPropertyType)propertyId);
    auto reply = m_impl->call<pb::AccountInfoIntegerReply>("AccountInfoInteger",
        [&](grpc::ClientContext& ctx, pb::AccountInfoIntegerReply& r) { return m_impl->accInfo->AccountInfoInteger(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().requested_value();
}

std::string MT5Client::accountInfoString(int propertyId) {
    pb::AccountInfoStringRequest req; req.set_property_id((pb::AccountInfoStringPropertyType)propertyId);
    auto reply = m_impl->call<pb::AccountInfoStringReply>("AccountInfoString",
        [&](grpc::ClientContext& ctx, pb::AccountInfoStringReply& r) { return m_impl->accInfo->AccountInfoString(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().requested_value();
}

// --- symbols --------------------------------------------------------------------------
bool MT5Client::symbolSelect(const std::string& symbol, bool select) {
    pb::SymbolSelectRequest req; req.set_symbol(symbol); req.set_select(select);
    auto reply = m_impl->call<pb::SymbolSelectReply>("SymbolSelect",
        [&](grpc::ClientContext& ctx, pb::SymbolSelectReply& r) { return m_impl->market->SymbolSelect(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().success();
}

bool MT5Client::symbolExist(const std::string& symbol) {
    pb::SymbolExistRequest req; req.set_name(symbol);
    auto reply = m_impl->call<pb::SymbolExistReply>("SymbolExist",
        [&](grpc::ClientContext& ctx, pb::SymbolExistReply& r) { return m_impl->market->SymbolExist(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().exists();
}

bool MT5Client::symbolIsSynchronized(const std::string& symbol) {
    pb::SymbolIsSynchronizedRequest req; req.set_symbol(symbol);
    auto reply = m_impl->call<pb::SymbolIsSynchronizedReply>("SymbolIsSynchronized",
        [&](grpc::ClientContext& ctx, pb::SymbolIsSynchronizedReply& r) { return m_impl->market->SymbolIsSynchronized(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().synchronized();
}

std::vector<std::string> MT5Client::symbolList() {
    pb::SymbolListRequest req;
    auto reply = m_impl->call<pb::SymbolListReply>("SymbolList",
        [&](grpc::ClientContext& ctx, pb::SymbolListReply& r) { return m_impl->market->SymbolList(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    std::vector<std::string> out;
    for (int i = 0; i < reply.data().symbols_size(); ++i) out.push_back(reply.data().symbols(i));
    return out;
}

Tick MT5Client::symbolInfoTick(const std::string& symbol) {
    pb::SymbolInfoTickRequest req; req.set_symbol(symbol);
    auto reply = m_impl->call<pb::SymbolInfoTickRequestReply>("SymbolInfoTick",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoTickRequestReply& r) { return m_impl->market->SymbolInfoTick(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    Tick t = tickFrom(reply.data(), symbol);
    t.time = m_impl->out(t.time);
    t.timeMsc = m_impl->outMs(t.timeMsc);
    return t;
}

std::vector<SymbolInfo> MT5Client::symbolParamsMany(const std::string& nameFilter) {
    pb::SymbolParamsManyRequest req;
    if (!nameFilter.empty()) req.set_symbol_name(nameFilter);
    auto reply = m_impl->call<pb::SymbolParamsManyReply>("SymbolParamsMany",
        [&](grpc::ClientContext& ctx, pb::SymbolParamsManyReply& r) { return m_impl->helper->SymbolParamsMany(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    std::vector<SymbolInfo> out;
    for (int i = 0; i < reply.data().symbol_infos_size(); ++i) {
        SymbolInfo s = symbolFrom(reply.data().symbol_infos(i));
        s.time = m_impl->out(s.time);
        out.push_back(s);
    }
    return out;
}

SymbolInfo MT5Client::symbolParams(const std::string& symbol) {
    auto all = symbolParamsMany(symbol);
    for (const auto& s : all) if (s.name == symbol) return s;
    throw ApiError("SYMBOL_NOT_FOUND", "SymbolParamsMany returned no entry named '" + symbol + "' (" +
                   std::to_string(all.size()) + " partial matches)", 0, 0, "", "SymbolParamsMany");
}

double MT5Client::symbolInfoDouble(const std::string& symbol, int propertyId) {
    pb::SymbolInfoDoubleRequest req; req.set_symbol(symbol); req.set_type((pb::SymbolInfoDoubleProperty)propertyId);
    auto reply = m_impl->call<pb::SymbolInfoDoubleReply>("SymbolInfoDouble",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoDoubleReply& r) { return m_impl->market->SymbolInfoDouble(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().value();
}

int64_t MT5Client::symbolInfoInteger(const std::string& symbol, int propertyId) {
    pb::SymbolInfoIntegerRequest req; req.set_symbol(symbol); req.set_type((pb::SymbolInfoIntegerProperty)propertyId);
    auto reply = m_impl->call<pb::SymbolInfoIntegerReply>("SymbolInfoInteger",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoIntegerReply& r) { return m_impl->market->SymbolInfoInteger(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().value();
}

std::string MT5Client::symbolInfoString(const std::string& symbol, int propertyId) {
    pb::SymbolInfoStringRequest req; req.set_symbol(symbol); req.set_type((pb::SymbolInfoStringProperty)propertyId);
    auto reply = m_impl->call<pb::SymbolInfoStringReply>("SymbolInfoString",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoStringReply& r) { return m_impl->market->SymbolInfoString(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().value();
}

// --- bars -----------------------------------------------------------------------------
std::vector<Bar> MT5Client::priceHistory(const std::string& symbol, Timeframe tf, int64_t fromUnix, int64_t toUnix, int maxBars) {
    pb::PriceHistoryRequest req;
    req.set_symbol(symbol);
    req.set_timeframe((int32_t)tf);
    // from/to are broker server time on the wire; the SDK takes UTC (see ClientOptions::utcTimes).
    req.set_from_time(m_impl->in(fromUnix));
    req.set_to_time(m_impl->in(toUnix));
    if (maxBars > 0) req.set_max_bars(maxBars);
    auto reply = m_impl->call<pb::PriceHistoryReply>("PriceHistory",
        [&](grpc::ClientContext& ctx, pb::PriceHistoryReply& r) { return m_impl->market->PriceHistory(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    std::vector<Bar> out;
    out.reserve(reply.data().bars_size());
    for (int i = 0; i < reply.data().bars_size(); ++i) {
        const auto& b = reply.data().bars(i);
        Bar o; o.time = m_impl->out(b.time()); o.open = b.open(); o.high = b.high(); o.low = b.low(); o.close = b.close();
        o.tickVolume = b.tick_volume(); o.realVolume = b.real_volume(); o.spread = b.spread();
        out.push_back(o);
    }
    std::sort(out.begin(), out.end(), [](const Bar& a, const Bar& b) { return a.time < b.time; });
    return out;
}

std::vector<Bar> MT5Client::lastBars(const std::string& symbol, Timeframe tf, int count) {
    // from_time == to_time == 0 + max_bars = "the newest `count` bars" (documented in the proto and
    // verified on prod). Never compute a window from the local clock here: the wire is server time.
    auto bars = priceHistory(symbol, tf, 0, 0, count);
    if ((int)bars.size() > count) bars.erase(bars.begin(), bars.end() - count);
    return bars;
}

// --- streaming ------------------------------------------------------------------------
std::unique_ptr<TickStream> MT5Client::subscribeTicks(const std::vector<std::string>& symbols,
                                                      std::function<void(const Tick&)> callback) {
    m_impl->ensureChannel();
    std::unique_ptr<TickStream> s(new TickStream());
    TickStream::Impl* im = s->m_impl.get();
    im->cb = std::move(callback);
    m_impl->fillContext(im->ctx, 0);
    pb::OnSymbolTickRequest req;
    for (const auto& sym : symbols) req.add_symbol_names(sym);
    // Learn the server offset before the stream starts so ticks can be normalised to UTC.
    const int64_t shift = m_impl->opts.utcTimes ? m_impl->offset() : 0;
    im->reader = m_impl->subs->OnSymbolTick(&im->ctx, req);
    im->running = true;
    im->th = std::thread([im, shift]() {
        pb::OnSymbolTickReply r;
        while (im->reader->Read(&r)) {
            if (replyHasError(r)) {
                std::lock_guard<std::mutex> lk(im->mu);
                im->err = r.error().error_code() + ": " + r.error().error_message();
                break;
            }
            if (r.has_data() && r.data().has_symbol_tick()) {
                Tick t = tickFrom(r.data().symbol_tick());
                if (shift && t.time) t.time -= shift;
                if (shift && t.timeMsc) t.timeMsc -= shift * 1000;
                im->count++;
                if (im->cb) {
                    try { im->cb(t); } catch (...) {}
                }
            }
        }
        grpc::Status st = im->reader->Finish();
        if (!st.ok() && st.error_code() != grpc::StatusCode::CANCELLED) {
            std::lock_guard<std::mutex> lk(im->mu);
            if (im->err.empty()) im->err = "grpc status " + std::to_string((int)st.error_code()) + ": " + st.error_message();
        }
        im->running = false;
    });
    return s;
}

uint64_t MT5Client::subscribeQuotes(const std::vector<std::string>& symbols,
                                    std::function<void(const Tick&)> callback, int durationMs) {
    auto s = subscribeTicks(symbols, std::move(callback));
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(durationMs);
    while (s->running() && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    s->stop();
    return s->count();
}

// --- trading --------------------------------------------------------------------------
OrderResult MT5Client::orderSend(const OrderRequest& q) {
    pb::OrderSendRequest req;
    req.set_symbol(q.symbol);
    req.set_operation((pb::TMT5_ENUM_ORDER_TYPE)(int)q.action);
    req.set_volume(q.volume);
    if (q.price > 0) req.set_price(q.price);
    req.set_slippage((uint64_t)std::max(0, q.slippage));
    if (q.stopLoss > 0) req.set_stop_loss(q.stopLoss);
    if (q.takeProfit > 0) req.set_take_profit(q.takeProfit);
    if (q.stopLimit > 0) req.set_stop_limit_price(q.stopLimit);
    if (!q.comment.empty()) req.set_comment(q.comment);
    if (q.magic != 0) req.set_expert_id((uint64_t)q.magic);
    if (q.expiration > 0) {
        req.set_expiration_time_type(pb::TMT5_ORDER_TIME_SPECIFIED);
        req.mutable_expiration_time()->set_seconds(m_impl->in(q.expiration));
    }
    auto reply = m_impl->call<pb::OrderSendReply>("OrderSend",
        [&](grpc::ClientContext& ctx, pb::OrderSendReply& r) { return m_impl->trade->OrderSend(&ctx, req, &r); },
        m_impl->opts.tradeTimeoutMs, true);
    return orderResultFrom(reply.data());
}

OrderResult MT5Client::orderModify(int64_t ticket, double stopLoss, double takeProfit, double price, double stopLimit) {
    pb::OrderModifyRequest req;
    req.set_ticket((uint64_t)ticket);
    req.set_stop_loss(stopLoss);
    req.set_take_profit(takeProfit);
    if (price > 0) req.set_price(price);
    if (stopLimit > 0) req.set_stop_limit(stopLimit);
    auto reply = m_impl->call<pb::OrderModifyReply>("OrderModify",
        [&](grpc::ClientContext& ctx, pb::OrderModifyReply& r) { return m_impl->trade->OrderModify(&ctx, req, &r); },
        m_impl->opts.tradeTimeoutMs, true);
    return orderResultFrom(reply.data());
}

OrderResult MT5Client::orderClose(int64_t ticket, double volume, int slippage) {
    if (volume <= 0) {
        auto open = openedOrders();
        for (const auto& p : open.positions) if (p.ticket == ticket) { volume = p.volume; break; }
        if (volume <= 0)
            for (const auto& o : open.orders) if (o.ticket == ticket) { volume = o.volumeCurrent; break; }
        if (volume <= 0)
            throw ApiError("POSITION_NOT_FOUND", "orderClose: ticket " + std::to_string(ticket) + " is not an open position or pending order",
                           0, 0, "", "OrderClose");
    }
    pb::OrderCloseRequest req;
    req.set_ticket((uint64_t)ticket);
    req.set_volume(volume);
    req.set_slippage(slippage);
    auto reply = m_impl->call<pb::OrderCloseReply>("OrderClose",
        [&](grpc::ClientContext& ctx, pb::OrderCloseReply& r) { return m_impl->trade->OrderClose(&ctx, req, &r); },
        m_impl->opts.tradeTimeoutMs, true);
    const auto& d = reply.data();
    OrderResult r;
    r.retcode = d.returned_code(); r.retcodeName = d.returned_string_code(); r.retcodeDescription = d.returned_code_description();
    r.ticket = ticket; r.order = ticket; r.volume = volume; r.comment = "close_mode=" + std::to_string((int)d.close_mode());
    return r;
}

OpenedOrders MT5Client::openedOrders() {
    pb::OpenedOrdersRequest req;
    req.set_inputsortmode(pb::BMT5_OPENED_ORDER_SORT_BY_OPEN_TIME_ASC);
    auto reply = m_impl->call<pb::OpenedOrdersReply>("OpenedOrders",
        [&](grpc::ClientContext& ctx, pb::OpenedOrdersReply& r) { return m_impl->helper->OpenedOrders(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    OpenedOrders out;
    for (int i = 0; i < reply.data().position_infos_size(); ++i) {
        Position p = positionFrom(reply.data().position_infos(i));
        p.openTime = m_impl->out(p.openTime);
        out.positions.push_back(p);
    }
    for (int i = 0; i < reply.data().opened_orders_size(); ++i) {
        PendingOrder o = pendingFrom(reply.data().opened_orders(i));
        o.timeSetup = m_impl->out(o.timeSetup);
        o.timeExpiration = m_impl->out(o.timeExpiration);
        out.orders.push_back(o);
    }
    return out;
}

std::vector<Position> MT5Client::positions(int64_t magic) {
    auto open = openedOrders();
    if (magic < 0) return open.positions;
    std::vector<Position> out;
    for (const auto& p : open.positions) if (p.magic == magic) out.push_back(p);
    return out;
}

int MT5Client::positionsTotal() {
    ::google::protobuf::Empty req;
    auto reply = m_impl->call<pb::PositionsTotalReply>("PositionsTotal",
        [&](grpc::ClientContext& ctx, pb::PositionsTotalReply& r) { return m_impl->tradeFn->PositionsTotal(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data().total_positions();
}

// --- history --------------------------------------------------------------------------
std::vector<HistoryItem> MT5Client::orderHistory(int64_t fromUnix, int64_t toUnix) {
    pb::OrderHistoryRequest req;
    // inputFrom/inputTo are broker server time on the wire ("required (server time)" in the proto).
    req.mutable_inputfrom()->set_seconds(m_impl->in(fromUnix));
    req.mutable_inputto()->set_seconds(m_impl->in(toUnix));
    req.set_inputsortmode(pb::BMT5_SORT_BY_CLOSE_TIME_ASC);
    req.set_pagenumber(0);
    req.set_itemsperpage(0);
    auto reply = m_impl->call<pb::OrderHistoryReply>("OrderHistory",
        [&](grpc::ClientContext& ctx, pb::OrderHistoryReply& r) { return m_impl->helper->OrderHistory(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    std::vector<HistoryItem> out;
    for (int i = 0; i < reply.data().history_data_size(); ++i) {
        const auto& h = reply.data().history_data(i);
        HistoryItem item;
        if (h.has_history_order()) {
            item.hasOrder = true; item.order = historyOrderFrom(h.history_order());
            item.order.setupTime = m_impl->out(item.order.setupTime);
            item.order.doneTime = m_impl->out(item.order.doneTime);
        }
        if (h.has_history_deal()) {
            item.hasDeal = true; item.deal = dealFrom(h.history_deal());
            item.deal.time = m_impl->out(item.deal.time);
        }
        out.push_back(item);
    }
    return out;
}

std::vector<PositionHistory> MT5Client::positionsHistory(int64_t fromUnix, int64_t toUnix) {
    pb::PositionsHistoryRequest req;
    req.mutable_position_open_time_from()->set_seconds(m_impl->in(fromUnix));
    req.mutable_position_open_time_to()->set_seconds(m_impl->in(toUnix));
    req.set_sort_type(pb::AH_POSITION_OPEN_TIME_ASC);
    auto reply = m_impl->call<pb::PositionsHistoryReply>("PositionsHistory",
        [&](grpc::ClientContext& ctx, pb::PositionsHistoryReply& r) { return m_impl->helper->PositionsHistory(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    std::vector<PositionHistory> out;
    for (int i = 0; i < reply.data().history_positions_size(); ++i) {
        const auto& p = reply.data().history_positions(i);
        PositionHistory o;
        o.ticket = (int64_t)p.position_ticket(); o.symbol = p.symbol(); o.orderType = (int)p.order_type(); o.volume = p.volume();
        o.openPrice = p.open_price(); o.closePrice = p.close_price(); o.stopLoss = p.stop_loss(); o.takeProfit = p.take_profit();
        o.profit = p.profit(); o.swap = p.swap(); o.commission = p.commission(); o.fee = p.fee();
        o.openTime = m_impl->out(p.open_time().seconds()); o.closeTime = m_impl->out(p.close_time().seconds()); o.magic = p.magic(); o.comment = p.comment();
        out.push_back(o);
    }
    return out;
}

} // namespace metarpc
