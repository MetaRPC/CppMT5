// MetaRPC CppMT5 — Layer 1 (MT5Account) real gRPC implementation over the generated mt5_term_api stubs.
#include "metarpc/mt5_account.hpp"

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
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>

namespace metarpc {

// ======================================================================================
// Hashing helpers (SHA-256 for the legacy id, SHA-1 for RFC 4122 uuid5) + misc
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

} // namespace

// ======================================================================================
// Static id helpers
// ======================================================================================
// RFC 4122 v5: SHA-1 over (namespace bytes || name), then stamp the version (5) and variant bits.
std::string MT5Account::uuid5(const std::string& namespaceUuid, const std::string& name) {
    std::vector<uint8_t> data = parseUuid(namespaceUuid);
    data.insert(data.end(), name.begin(), name.end());
    std::vector<uint8_t> h = sha1Bytes(data);
    h[6] = (uint8_t)((h[6] & 0x0F) | 0x50);
    h[8] = (uint8_t)((h[8] & 0x3F) | 0x80);
    return formatUuid(h.data());
}

// Legacy id: SHA-256 of "login:password", then reorder the first 8 bytes to the little-endian
// layout the fleet historically used before formatting as a UUID string.
std::string MT5Account::computeDeterministicId(int64_t login, const std::string& password) {
    std::string input = std::to_string(login) + ":" + password;
    std::vector<uint8_t> h = sha256Bytes(input);
    uint8_t le[16];
    le[0] = h[3]; le[1] = h[2]; le[2] = h[1]; le[3] = h[0];
    le[4] = h[5]; le[5] = h[4]; le[6] = h[7]; le[7] = h[6];
    for (int i = 8; i < 16; ++i) le[i] = h[i];
    return formatUuid(le);
}

// ======================================================================================
// AccountStream (generic server-stream handle) + the templated pump that feeds it
// ======================================================================================
// The handle is deliberately type-agnostic: it only holds the context (to cancel), the reader
// thread and the counters/error. The payload type lives entirely inside the thread spawned by
// pumpStream(), so one handle class serves every On* stream.
struct AccountStream::Impl {
    grpc::ClientContext ctx;
    std::thread th;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> count{0};
    mutable std::mutex mu;
    std::string err;
};

AccountStream::AccountStream() : m_impl(new Impl()) {}
AccountStream::~AccountStream() { stop(); }

void AccountStream::stop() {
    if (!m_impl) return;
    if (m_impl->th.joinable()) {
        m_impl->ctx.TryCancel();
        m_impl->th.join();
    }
    m_impl->running = false;
}
bool AccountStream::running() const { return m_impl && m_impl->running.load(); }
uint64_t AccountStream::count() const { return m_impl ? m_impl->count.load() : 0; }
std::string AccountStream::lastError() const {
    if (!m_impl) return "";
    std::lock_guard<std::mutex> lk(m_impl->mu);
    return m_impl->err;
}

namespace {
// Spawn the background reader thread for an already-opened stream. Reply is the wire reply type,
// Data is reply.data()'s type, Reader is the gRPC ClientReader(Interface) held by value. The
// loop reads frames, stops on an error payload, and delivers every data payload to `cb`.
template <class Reply, class Data, class Reader>
void pumpStream(AccountStream::Impl* im, Reader reader, std::function<void(const Data&)> cb) {
    im->running = true;
    im->th = std::thread([im, reader = std::move(reader), cb = std::move(cb)]() mutable {
        Reply r;
        // Read() blocks until the next frame and returns false when the stream ends.
        while (reader->Read(&r)) {
            if (replyHasError(r)) {           // an error frame ends the stream
                std::lock_guard<std::mutex> lk(im->mu);
                im->err = r.error().error_code() + ": " + r.error().error_message();
                break;
            }
            if (r.has_data()) {               // deliver the payload; a throwing cb must not kill us
                im->count++;
                if (cb) { try { cb(r.data()); } catch (...) {} }
            }
        }
        grpc::Status st = reader->Finish();   // a clean cancel shows up as CANCELLED, not an error
        if (!st.ok() && st.error_code() != grpc::StatusCode::CANCELLED) {
            std::lock_guard<std::mutex> lk(im->mu);
            if (im->err.empty()) im->err = "grpc status " + std::to_string((int)st.error_code()) + ": " + st.error_message();
        }
        im->running = false;
    });
}
} // namespace

// ======================================================================================
// MT5Account::Impl
// ======================================================================================
struct MT5Account::Impl {
    std::string host;
    int port;
    std::string apiKey;
    std::string name;
    std::string id;             // header "id"
    std::string terminalGuid;   // what ConnectEx returned
    int64_t login{0};
    std::string password;
    std::string server;
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
    std::atomic<int64_t> offsetSec{0};
    std::atomic<bool> offsetKnown{false};

    void log(const std::string& s) {
        if (opts.verbose) std::cerr << "[cppmt5] " << s << std::endl;
    }

    // Lazily create the gRPC channel and all seven service stubs on first use. Picks TLS for
    // 443/8443 (loading a CA bundle from rootCertsPath or a common OS location) and plaintext
    // otherwise, and sets keepalive + a large max-receive size for big history replies.
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

    // Attach the per-call metadata the gateway expects — the terminal "id" header (when set)
    // and the "apikey" header ("TRIAL" if none) — and set the attempt deadline. Called once
    // per attempt so each retry gets a fresh deadline.
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

    // The core transport pattern, shared by every RPC on this client. It wraps a single gRPC
    // invocation (the `fn` closure) with three behaviours:
    //   1. transient-failure retry  — UNAVAILABLE / DEADLINE_EXCEEDED are retried with
    //      exponential back-off up to opts.maxRetries;
    //   2. auto re-attach           — a TERMINAL_INSTANCE_NOT_FOUND error payload re-issues
    //      ConnectEx (when allowReconnect) and retries, so a terminal restart is transparent;
    //   3. error mapping            — any other error payload becomes an ApiError, and an
    //      exhausted transport failure becomes a TransportError.
    // `what` is the RPC name used in logs and error messages. Returns the successful Reply.
    template <class Reply, class Fn>
    Reply call(const char* what, Fn&& fn, int timeoutMs, bool allowReconnect) {
        ensureChannel();                       // Step 0: lazily open the channel + stubs
        int attempt = 0;
        int delayMs = 500;
        for (;;) {
            // Step 1: build a fresh context (deadline + id/apikey metadata) for this attempt.
            grpc::ClientContext ctx;
            fillContext(ctx, timeoutMs);

            // Step 2: run the actual unary RPC supplied by the caller.
            Reply reply;
            grpc::Status st = fn(ctx, reply);

            // Step 3: transport-level failure? Retry transient ones, otherwise give up hard.
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

            // Step 4: the call reached the server but it answered with an error payload.
            if (replyHasError(reply)) {
                const pb::Error& e = reply.error();
                // Step 4a: a lost terminal is recoverable — re-attach via ConnectEx and retry.
                if (allowReconnect && attempt < opts.maxRetries &&
                    (e.error_code() == "TERMINAL_INSTANCE_NOT_FOUND" || e.error_code() == "TERMINAL_REGISTRY_TERMINAL_NOT_FOUND")) {
                    log(std::string(what) + ": " + e.error_code() + " -> reconnect");
                    ++attempt;
                    connectEx();
                    sleepBackoff(delayMs);
                    continue;
                }
                // Step 4b: any other error is final — surface it as a typed ApiError.
                throw makeApiError(e, what);
            }

            // Step 5: success.
            return reply;
        }
    }

    // Issue the ConnectEx RPC with the stored credentials and adopt the terminal id the
    // gateway hands back (it may differ from the one we requested). This is both the initial
    // connect and the auto-reconnect path, so it must not itself allow reconnect (no recursion).
    void connectEx() {
        if (login == 0 || server.empty()) throw ApiError("NOT_CONFIGURED", "connect() was never called", 0, 0, "", "ConnectEx");
        pb::ConnectExRequest req;
        req.set_user((uint64_t)login);
        req.set_password(password);
        req.set_mt_cluster_name(server);
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

    // The broker-clock-minus-UTC offset in seconds, learned once from AccountSummary and then
    // cached. Returned immediately on subsequent calls. Layer 2 uses it for UTC conversion.
    int64_t offset() {
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
};

// ======================================================================================
// MT5Account — lifecycle / identity / connection
// ======================================================================================
#pragma region Lifecycle / identity / connection
MT5Account::MT5Account(const std::string& host, int port, const std::string& apiKey, const std::string& name)
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

// The destructor does NOT send Disconnect: the gateway keeps the terminal alive for the
// stable id, which is what a 24/7 strategy wants across restarts.
MT5Account::~MT5Account() { close(); }

std::string MT5Account::getId(int64_t login, const std::string& password) {
    std::lock_guard<std::mutex> lk(m_impl->mu);
    m_impl->id = computeDeterministicId(login, password);
    return m_impl->id;
}
void MT5Account::setId(const std::string& id) { std::lock_guard<std::mutex> lk(m_impl->mu); m_impl->id = id; }
const std::string& MT5Account::getId() const { return m_impl->id; }
void MT5Account::setApiKey(const std::string& apiKey) { m_impl->apiKey = apiKey; }
const std::string& MT5Account::getApiKey() const { return m_impl->apiKey; }
void MT5Account::setName(const std::string& name) { m_impl->name = name; }
const std::string& MT5Account::getName() const { return m_impl->name; }
ClientOptions& MT5Account::options() { return m_impl->opts; }
std::string MT5Account::terminalGuid() const { std::lock_guard<std::mutex> lk(m_impl->mu); return m_impl->terminalGuid; }
bool MT5Account::isConnected() const { return m_impl->connected.load(); }
int64_t MT5Account::serverOffsetSeconds() { return m_impl->offset(); }

// Store the credentials, derive a terminal id if none was set, open the channel and run
// ConnectEx. Blocks until the terminal is up (see connectEx). Throws on failure.
void MT5Account::connect(int64_t login, const std::string& password, const std::string& server,
                         const std::string& name, int timeoutSeconds) {
    // Step 1: remember the credentials so the auto-reconnect path can reuse them.
    m_impl->login = login;
    m_impl->password = password;
    m_impl->server = server;
    m_impl->connectTimeoutSeconds = timeoutSeconds;
    if (!name.empty()) m_impl->name = name;
    // Step 2: if the caller never set an id, derive a stable one from the credentials.
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        if (m_impl->id.empty()) m_impl->id = computeDeterministicId(login, password);
    }
    // Step 3: open the channel/stubs and attach the terminal.
    m_impl->ensureChannel();
    m_impl->connectEx();
}

// Re-run ConnectEx with the stored credentials (e.g. after a manual close()).
void MT5Account::reconnect() { m_impl->connectEx(); }

// Tell the gateway to STOP the terminal. Best-effort: the connected flag is cleared even if the
// RPC throws, and a missing channel/id is treated as already-disconnected.
void MT5Account::disconnect(const std::string& reason) {
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

// Drop the local channel and stubs; the terminal keeps running on the gateway. A later
// connect()/reconnect() rebuilds them (ensureChannel) and re-attaches to the same terminal.
void MT5Account::close() {
    m_impl->connected = false;
    m_impl->conn.reset(); m_impl->helper.reset(); m_impl->accInfo.reset(); m_impl->market.reset();
    m_impl->subs.reset(); m_impl->trade.reset(); m_impl->tradeFn.reset();
    m_impl->channel.reset();
}

#pragma endregion

// ======================================================================================
// MT5Account — RPCs (protobuf in / protobuf out, no conversion, no UTC shift)
// ======================================================================================
#pragma region Account
// Full account snapshot in one round-trip. Besides the account fields, the reply carries the
// broker's UTC offset, which we cache here as a side effect so later time conversions are free.
pb::AccountSummaryData MT5Account::accountSummary(const pb::AccountSummaryRequest& req) {
    // Step 1: issue the AccountSummary RPC on the AccountHelper stub. call<> adds the per-call
    // deadline, transient-failure retry and automatic re-attach on a lost terminal.
    auto reply = m_impl->call<pb::AccountSummaryReply>("AccountSummary",
        [&](grpc::ClientContext& ctx, pb::AccountSummaryReply& r) { return m_impl->helper->AccountSummary(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: cache the broker-clock-minus-UTC offset so Layer 2 can convert times without
    // spending another AccountSummary round-trip.
    m_impl->offsetSec = reply.data().utc_timezone_server_time_shift_minutes() * 60;
    m_impl->offsetKnown = true;
    // Step 3: return the raw protobuf payload unchanged (this layer does no conversion).
    return reply.data();
}

// One double-typed account property (balance, equity, margin, …) selected by req.property_id.
pb::AccountInfoDoubleData MT5Account::accountInfoDouble(const pb::AccountInfoDoubleRequest& req) {
    // Step 1: issue the AccountInfoDouble RPC on the AccountInformation stub (retried by call<>).
    auto reply = m_impl->call<pb::AccountInfoDoubleReply>("AccountInfoDouble",
        [&](grpc::ClientContext& ctx, pb::AccountInfoDoubleReply& r) { return m_impl->accInfo->AccountInfoDouble(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload; the number is in its requested_value field.
    return reply.data();
}

// One integer-typed account property (login, leverage, trade mode, …) selected by req.property_id.
pb::AccountInfoIntegerData MT5Account::accountInfoInteger(const pb::AccountInfoIntegerRequest& req) {
    // Step 1: issue the AccountInfoInteger RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::AccountInfoIntegerReply>("AccountInfoInteger",
        [&](grpc::ClientContext& ctx, pb::AccountInfoIntegerReply& r) { return m_impl->accInfo->AccountInfoInteger(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload; the number is in its requested_value field.
    return reply.data();
}

// One string-typed account property (name, server, currency, company) selected by req.property_id.
pb::AccountInfoStringData MT5Account::accountInfoString(const pb::AccountInfoStringRequest& req) {
    // Step 1: issue the AccountInfoString RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::AccountInfoStringReply>("AccountInfoString",
        [&](grpc::ClientContext& ctx, pb::AccountInfoStringReply& r) { return m_impl->accInfo->AccountInfoString(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload; the string is in its requested_value field.
    return reply.data();
}

#pragma endregion

#pragma region Symbols / quotes
// Add or remove a symbol from Market Watch (req.select). Returns success in the payload.
pb::SymbolSelectData MT5Account::symbolSelect(const pb::SymbolSelectRequest& req) {
    // Step 1: issue the SymbolSelect RPC on the MarketInfo stub (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolSelectReply>("SymbolSelect",
        [&](grpc::ClientContext& ctx, pb::SymbolSelectReply& r) { return m_impl->market->SymbolSelect(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (its success field reports the outcome).
    return reply.data();
}

// Whether the broker knows req.name at all; the payload also flags custom symbols.
pb::SymbolExistData MT5Account::symbolExist(const pb::SymbolExistRequest& req) {
    // Step 1: issue the SymbolExist RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolExistReply>("SymbolExist",
        [&](grpc::ClientContext& ctx, pb::SymbolExistReply& r) { return m_impl->market->SymbolExist(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (exists / is_custom).
    return reply.data();
}

// Whether a symbol's rates/params are fully synchronised on the terminal yet.
pb::SymbolIsSynchronizedData MT5Account::symbolIsSynchronized(const pb::SymbolIsSynchronizedRequest& req) {
    // Step 1: issue the SymbolIsSynchronized RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolIsSynchronizedReply>("SymbolIsSynchronized",
        [&](grpc::ClientContext& ctx, pb::SymbolIsSynchronizedReply& r) { return m_impl->market->SymbolIsSynchronized(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (its synchronized field holds the answer).
    return reply.data();
}

// Every symbol name known to the terminal (the whole broker list, not just Market Watch).
pb::SymbolListData MT5Account::symbolList(const pb::SymbolListRequest& req) {
    // Step 1: issue the SymbolList RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolListReply>("SymbolList",
        [&](grpc::ClientContext& ctx, pb::SymbolListReply& r) { return m_impl->market->SymbolList(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (names are in its repeated symbols field).
    return reply.data();
}

// Latest tick for req.symbol. Note the irregular reply type (SymbolInfoTickRequestReply) whose
// data() is a MrpcMqlTick; its time fields are raw broker server time (Layer 2 shifts to UTC).
pb::MrpcMqlTick MT5Account::symbolInfoTick(const pb::SymbolInfoTickRequest& req) {
    // Step 1: issue the SymbolInfoTick RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolInfoTickRequestReply>("SymbolInfoTick",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoTickRequestReply& r) { return m_impl->market->SymbolInfoTick(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the tick message directly.
    return reply.data();
}

// One double symbol property selected by req.type (the PROTO SymbolInfoDoubleProperty enum).
pb::SymbolInfoDoubleData MT5Account::symbolInfoDouble(const pb::SymbolInfoDoubleRequest& req) {
    // Step 1: issue the SymbolInfoDouble RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolInfoDoubleReply>("SymbolInfoDouble",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoDoubleReply& r) { return m_impl->market->SymbolInfoDouble(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (number is in its value field).
    return reply.data();
}

// One integer symbol property selected by req.type (the PROTO SymbolInfoIntegerProperty enum).
pb::SymbolInfoIntegerData MT5Account::symbolInfoInteger(const pb::SymbolInfoIntegerRequest& req) {
    // Step 1: issue the SymbolInfoInteger RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolInfoIntegerReply>("SymbolInfoInteger",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoIntegerReply& r) { return m_impl->market->SymbolInfoInteger(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (number is in its value field).
    return reply.data();
}

// One string symbol property selected by req.type (the PROTO SymbolInfoStringProperty enum).
pb::SymbolInfoStringData MT5Account::symbolInfoString(const pb::SymbolInfoStringRequest& req) {
    // Step 1: issue the SymbolInfoString RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolInfoStringReply>("SymbolInfoString",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoStringReply& r) { return m_impl->market->SymbolInfoString(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (string is in its value field).
    return reply.data();
}

// Full parameter set for one or many symbols in a single call. Runs on the AccountHelper stub
// and can return a lot of data, so it gets a doubled deadline.
pb::SymbolParamsManyData MT5Account::symbolParamsMany(const pb::SymbolParamsManyRequest& req) {
    // Step 1: issue the SymbolParamsMany RPC (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::SymbolParamsManyReply>("SymbolParamsMany",
        [&](grpc::ClientContext& ctx, pb::SymbolParamsManyReply& r) { return m_impl->helper->SymbolParamsMany(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    // Step 2: return the data payload (its repeated symbol_infos holds one entry per symbol).
    return reply.data();
}

// Count of symbols (all, or Market Watch only, per req.mode).
pb::SymbolsTotalData MT5Account::symbolsTotal(const pb::SymbolsTotalRequest& req) {
    auto reply = m_impl->call<pb::SymbolsTotalReply>("SymbolsTotal",
        [&](grpc::ClientContext& ctx, pb::SymbolsTotalReply& r) { return m_impl->market->SymbolsTotal(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();  // total
}

// Symbol name by index — pair with symbolsTotal() to enumerate symbols.
pb::SymbolNameData MT5Account::symbolName(const pb::SymbolNameRequest& req) {
    auto reply = m_impl->call<pb::SymbolNameReply>("SymbolName",
        [&](grpc::ClientContext& ctx, pb::SymbolNameReply& r) { return m_impl->market->SymbolName(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();  // name
}

// Initial/maintenance margin rate for a symbol + order type.
pb::SymbolInfoMarginRateData MT5Account::symbolInfoMarginRate(const pb::SymbolInfoMarginRateRequest& req) {
    auto reply = m_impl->call<pb::SymbolInfoMarginRateReply>("SymbolInfoMarginRate",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoMarginRateReply& r) { return m_impl->market->SymbolInfoMarginRate(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Quote-session times for a symbol on a weekday/session index (raw broker server time).
pb::SymbolInfoSessionQuoteData MT5Account::symbolInfoSessionQuote(const pb::SymbolInfoSessionQuoteRequest& req) {
    auto reply = m_impl->call<pb::SymbolInfoSessionQuoteReply>("SymbolInfoSessionQuote",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoSessionQuoteReply& r) { return m_impl->market->SymbolInfoSessionQuote(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Trade-session times for a symbol on a weekday/session index (raw broker server time).
pb::SymbolInfoSessionTradeData MT5Account::symbolInfoSessionTrade(const pb::SymbolInfoSessionTradeRequest& req) {
    auto reply = m_impl->call<pb::SymbolInfoSessionTradeReply>("SymbolInfoSessionTrade",
        [&](grpc::ClientContext& ctx, pb::SymbolInfoSessionTradeReply& r) { return m_impl->market->SymbolInfoSessionTrade(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Tick value + contract size for several symbols at once. Lives on the AccountHelper stub.
pb::TickValueWithSizeData MT5Account::tickValueWithSize(const pb::TickValueWithSizeRequest& req) {
    auto reply = m_impl->call<pb::TickValueWithSizeReply>("TickValueWithSize",
        [&](grpc::ClientContext& ctx, pb::TickValueWithSizeReply& r) { return m_impl->helper->TickValueWithSize(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

#pragma endregion

#pragma region Market depth (DOM)
// Subscribe the terminal to a symbol's order book; required before marketBookGet() returns data.
pb::MarketBookAddData MT5Account::marketBookAdd(const pb::MarketBookAddRequest& req) {
    auto reply = m_impl->call<pb::MarketBookAddReply>("MarketBookAdd",
        [&](grpc::ClientContext& ctx, pb::MarketBookAddReply& r) { return m_impl->market->MarketBookAdd(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Current order-book snapshot (price levels) for a subscribed symbol.
pb::MarketBookGetData MT5Account::marketBookGet(const pb::MarketBookGetRequest& req) {
    auto reply = m_impl->call<pb::MarketBookGetReply>("MarketBookGet",
        [&](grpc::ClientContext& ctx, pb::MarketBookGetReply& r) { return m_impl->market->MarketBookGet(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Unsubscribe the terminal from a symbol's order book.
pb::MarketBookReleaseData MT5Account::marketBookRelease(const pb::MarketBookReleaseRequest& req) {
    auto reply = m_impl->call<pb::MarketBookReleaseReply>("MarketBookRelease",
        [&](grpc::ClientContext& ctx, pb::MarketBookReleaseReply& r) { return m_impl->market->MarketBookRelease(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}
#pragma endregion

#pragma region Bars
// OHLC bars for req.symbol/req.timeframe over the req window. Both the window and the returned
// bar times are raw broker server time. Potentially large, so it gets a doubled deadline.
pb::PriceHistoryData MT5Account::priceHistory(const pb::PriceHistoryRequest& req) {
    // Step 1: issue the PriceHistory RPC on the MarketInfo stub (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::PriceHistoryReply>("PriceHistory",
        [&](grpc::ClientContext& ctx, pb::PriceHistoryReply& r) { return m_impl->market->PriceHistory(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    // Step 2: return the data payload (bars are in its repeated bars field, server-time order).
    return reply.data();
}

#pragma endregion

#pragma region Trading
// Place a market or pending order. Runs on the TradingHelper stub with the (longer) trade
// deadline. Note: a successful RPC does not mean a successful trade — inspect returned_code.
pb::OrderSendData MT5Account::orderSend(const pb::OrderSendRequest& req) {
    // Step 1: issue the OrderSend RPC (retried/reconnected by call<>, trade-length deadline).
    auto reply = m_impl->call<pb::OrderSendReply>("OrderSend",
        [&](grpc::ClientContext& ctx, pb::OrderSendReply& r) { return m_impl->trade->OrderSend(&ctx, req, &r); },
        m_impl->opts.tradeTimeoutMs, true);
    // Step 2: return the data payload (returned_code, deal/order tickets, fill price, …).
    return reply.data();
}

// Modify SL/TP (and price/stop-limit for pending orders) of an existing order/position.
pb::OrderModifyData MT5Account::orderModify(const pb::OrderModifyRequest& req) {
    // Step 1: issue the OrderModify RPC (retried/reconnected by call<>, trade-length deadline).
    auto reply = m_impl->call<pb::OrderModifyReply>("OrderModify",
        [&](grpc::ClientContext& ctx, pb::OrderModifyReply& r) { return m_impl->trade->OrderModify(&ctx, req, &r); },
        m_impl->opts.tradeTimeoutMs, true);
    // Step 2: return the data payload (check its returned_code).
    return reply.data();
}

// Close/partially close a position, or delete a pending order, by req.ticket.
pb::OrderCloseData MT5Account::orderClose(const pb::OrderCloseRequest& req) {
    // Step 1: issue the OrderClose RPC (retried/reconnected by call<>, trade-length deadline).
    auto reply = m_impl->call<pb::OrderCloseReply>("OrderClose",
        [&](grpc::ClientContext& ctx, pb::OrderCloseReply& r) { return m_impl->trade->OrderClose(&ctx, req, &r); },
        m_impl->opts.tradeTimeoutMs, true);
    // Step 2: return the data payload (returned_code + the close_mode the gateway applied).
    return reply.data();
}

// Snapshot of everything live on the account: open positions and pending orders.
pb::OpenedOrdersData MT5Account::openedOrders(const pb::OpenedOrdersRequest& req) {
    // Step 1: issue the OpenedOrders RPC on the AccountHelper stub (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::OpenedOrdersReply>("OpenedOrders",
        [&](grpc::ClientContext& ctx, pb::OpenedOrdersReply& r) { return m_impl->helper->OpenedOrders(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (repeated position_infos and opened_orders).
    return reply.data();
}

// Number of currently open positions. This RPC takes an Empty request and lives on the
// TradeFunctions stub (not AccountHelper).
pb::PositionsTotalData MT5Account::positionsTotal() {
    // Step 1: build the empty request and issue PositionsTotal (retried/reconnected by call<>).
    ::google::protobuf::Empty req;
    auto reply = m_impl->call<pb::PositionsTotalReply>("PositionsTotal",
        [&](grpc::ClientContext& ctx, pb::PositionsTotalReply& r) { return m_impl->tradeFn->PositionsTotal(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    // Step 2: return the data payload (count is in its total_positions field).
    return reply.data();
}

// Just the tickets of live positions and pending orders (lighter than openedOrders).
pb::OpenedOrdersTicketsData MT5Account::openedOrdersTickets(const pb::OpenedOrdersTicketsRequest& req) {
    auto reply = m_impl->call<pb::OpenedOrdersTicketsReply>("OpenedOrdersTickets",
        [&](grpc::ClientContext& ctx, pb::OpenedOrdersTicketsReply& r) { return m_impl->helper->OpenedOrdersTickets(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Validate an order without sending it (pre-flight). Lives on the TradeFunctions stub.
pb::OrderCheckData MT5Account::orderCheck(const pb::OrderCheckRequest& req) {
    auto reply = m_impl->call<pb::OrderCheckReply>("OrderCheck",
        [&](grpc::ClientContext& ctx, pb::OrderCheckReply& r) { return m_impl->tradeFn->OrderCheck(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Margin required to open a given order. Lives on the TradeFunctions stub.
pb::OrderCalcMarginData MT5Account::orderCalcMargin(const pb::OrderCalcMarginRequest& req) {
    auto reply = m_impl->call<pb::OrderCalcMarginReply>("OrderCalcMargin",
        [&](grpc::ClientContext& ctx, pb::OrderCalcMarginReply& r) { return m_impl->tradeFn->OrderCalcMargin(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

// Profit a price move would yield. Lives on the TradeFunctions stub.
pb::OrderCalcProfitData MT5Account::orderCalcProfit(const pb::OrderCalcProfitRequest& req) {
    auto reply = m_impl->call<pb::OrderCalcProfitReply>("OrderCalcProfit",
        [&](grpc::ClientContext& ctx, pb::OrderCalcProfitReply& r) { return m_impl->tradeFn->OrderCalcProfit(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs, true);
    return reply.data();
}

#pragma endregion

#pragma region History
// Closed orders and deals in the req window (raw broker server time on the wire). Histories can
// be large, hence the doubled deadline. Note the reply's data() is OrdersHistoryData (plural).
pb::OrdersHistoryData MT5Account::orderHistory(const pb::OrderHistoryRequest& req) {
    // Step 1: issue the OrderHistory RPC on the AccountHelper stub (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::OrderHistoryReply>("OrderHistory",
        [&](grpc::ClientContext& ctx, pb::OrderHistoryReply& r) { return m_impl->helper->OrderHistory(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    // Step 2: return the data payload (each history_data entry carries an order and/or a deal).
    return reply.data();
}

// Closed positions in the req window (raw broker server time on the wire).
pb::PositionsHistoryData MT5Account::positionsHistory(const pb::PositionsHistoryRequest& req) {
    // Step 1: issue the PositionsHistory RPC on the AccountHelper stub (retried/reconnected by call<>).
    auto reply = m_impl->call<pb::PositionsHistoryReply>("PositionsHistory",
        [&](grpc::ClientContext& ctx, pb::PositionsHistoryReply& r) { return m_impl->helper->PositionsHistory(&ctx, req, &r); },
        m_impl->opts.unaryTimeoutMs * 2, true);
    // Step 2: return the data payload (its repeated history_positions holds the closed positions).
    return reply.data();
}

#pragma endregion

// ======================================================================================
// MT5Account — streaming (raw protobuf tick payloads)
// ======================================================================================
#pragma region Streaming
// All five follow the same shape: ensure the channel, allocate the handle, open the typed server
// reader (no deadline — streams are open-ended), then hand it to pumpStream, which spawns the
// background reader thread and feeds the caller's callback. The AccountStream handle keeps the
// subscription alive until it is destroyed / stop()ed. Streams do NOT go through call<> — there
// is nothing to retry on a long-lived stream.

std::unique_ptr<AccountStream> MT5Account::onSymbolTick(
    const pb::OnSymbolTickRequest& req, std::function<void(const pb::OnSymbolTickData&)> callback) {
    m_impl->ensureChannel();
    std::unique_ptr<AccountStream> s(new AccountStream());
    AccountStream::Impl* im = s->m_impl.get();
    m_impl->fillContext(im->ctx, 0);
    pumpStream<pb::OnSymbolTickReply, pb::OnSymbolTickData>(
        im, m_impl->subs->OnSymbolTick(&im->ctx, req), std::move(callback));
    return s;
}

std::unique_ptr<AccountStream> MT5Account::onTrade(
    const pb::OnTradeRequest& req, std::function<void(const pb::OnTradeData&)> callback) {
    m_impl->ensureChannel();
    std::unique_ptr<AccountStream> s(new AccountStream());
    AccountStream::Impl* im = s->m_impl.get();
    m_impl->fillContext(im->ctx, 0);
    pumpStream<pb::OnTradeReply, pb::OnTradeData>(
        im, m_impl->subs->OnTrade(&im->ctx, req), std::move(callback));
    return s;
}

std::unique_ptr<AccountStream> MT5Account::onTradeTransaction(
    const pb::OnTradeTransactionRequest& req, std::function<void(const pb::OnTradeTransactionData&)> callback) {
    m_impl->ensureChannel();
    std::unique_ptr<AccountStream> s(new AccountStream());
    AccountStream::Impl* im = s->m_impl.get();
    m_impl->fillContext(im->ctx, 0);
    pumpStream<pb::OnTradeTransactionReply, pb::OnTradeTransactionData>(
        im, m_impl->subs->OnTradeTransaction(&im->ctx, req), std::move(callback));
    return s;
}

std::unique_ptr<AccountStream> MT5Account::onPositionProfit(
    const pb::OnPositionProfitRequest& req, std::function<void(const pb::OnPositionProfitData&)> callback) {
    m_impl->ensureChannel();
    std::unique_ptr<AccountStream> s(new AccountStream());
    AccountStream::Impl* im = s->m_impl.get();
    m_impl->fillContext(im->ctx, 0);
    pumpStream<pb::OnPositionProfitReply, pb::OnPositionProfitData>(
        im, m_impl->subs->OnPositionProfit(&im->ctx, req), std::move(callback));
    return s;
}

std::unique_ptr<AccountStream> MT5Account::onPositionsAndPendingOrdersTickets(
    const pb::OnPositionsAndPendingOrdersTicketsRequest& req,
    std::function<void(const pb::OnPositionsAndPendingOrdersTicketsData&)> callback) {
    m_impl->ensureChannel();
    std::unique_ptr<AccountStream> s(new AccountStream());
    AccountStream::Impl* im = s->m_impl.get();
    m_impl->fillContext(im->ctx, 0);
    pumpStream<pb::OnPositionsAndPendingOrdersTicketsReply, pb::OnPositionsAndPendingOrdersTicketsData>(
        im, m_impl->subs->OnPositionsAndPendingOrdersTickets(&im->ctx, req), std::move(callback));
    return s;
}
#pragma endregion

} // namespace metarpc
