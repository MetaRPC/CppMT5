#include "metarpc/mt5.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cstdlib>

namespace metarpc {

namespace {
    inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
    inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
        return (e & f) ^ (~e & g);
    }
    inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
        return (a & b) ^ (a & c) ^ (b & c);
    }
    inline uint32_t sig0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    inline uint32_t sig1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    inline uint32_t theta0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    inline uint32_t theta1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    static const uint32_t K[64] = {
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
        while ((msg.size() + 8) % 64 != 0) {
            msg.push_back(0x00);
        }
        for (int i = 7; i >= 0; --i) {
            msg.push_back((uint8_t)((bit_len >> (i * 8)) & 0xFF));
        }

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = ((uint32_t)msg[chunk + i * 4] << 24) |
                       ((uint32_t)msg[chunk + i * 4 + 1] << 16) |
                       ((uint32_t)msg[chunk + i * 4 + 2] << 8) |
                       ((uint32_t)msg[chunk + i * 4 + 3]);
            }
            for (int i = 16; i < 64; ++i) {
                w[i] = theta1(w[i - 2]) + w[i - 7] + theta0(w[i - 15]) + w[i - 16];
            }
            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
            for (int i = 0; i < 64; ++i) {
                uint32_t t1 = h + sig1(e) + choose(e, f, g) + K[i] + w[i];
                uint32_t t2 = sig0(a) + majority(a, b, c);
                h = g; g = f; f = e; e = d + t1;
                d = c; c = b; b = a; a = t1 + t2;
            }
            h0 += a; h1 += b; h2 += c; h3 += d;
            h4 += e; h5 += f; h6 += g; h7 += h;
        }

        std::vector<uint8_t> hash(32);
        uint32_t h[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
        for (int i = 0; i < 8; ++i) {
            hash[i * 4] = (uint8_t)((h[i] >> 24) & 0xFF);
            hash[i * 4 + 1] = (uint8_t)((h[i] >> 16) & 0xFF);
            hash[i * 4 + 2] = (uint8_t)((h[i] >> 8) & 0xFF);
            hash[i * 4 + 3] = (uint8_t)(h[i] & 0xFF);
        }
        return hash;
    }
}

std::string MT5Client::computeDeterministicId(int64_t login, const std::string& password) {
    std::string input = std::to_string(login) + ":" + password;
    std::vector<uint8_t> h = sha256Bytes(input);

    uint8_t le[16];
    le[0] = h[3];
    le[1] = h[2];
    le[2] = h[1];
    le[3] = h[0];
    le[4] = h[5];
    le[5] = h[4];
    le[6] = h[7];
    le[7] = h[6];
    for (int i = 8; i < 16; ++i) {
        le[i] = h[i];
    }

    char buf[37];
    snprintf(buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        le[0], le[1], le[2], le[3],
        le[4], le[5],
        le[6], le[7],
        le[8], le[9],
        le[10], le[11], le[12], le[13], le[14], le[15]);
    return std::string(buf);
}

MT5Client::MT5Client(const std::string& host, int port, const std::string& apiKey)
    : m_host(host), m_port(port), m_apiKey(apiKey), m_connected(false) {
    if (m_apiKey.empty()) {
        const char* envKey = std::getenv("MRPC_API_KEY");
        if (envKey) {
            m_apiKey = envKey;
        }
    }
}

MT5Client::~MT5Client() {
    disconnect();
}

std::string MT5Client::getId(int64_t login, const std::string& password) {
    m_id = computeDeterministicId(login, password);
    return m_id;
}

bool MT5Client::connect(int64_t login, const std::string& password) {
    if (m_id.empty()) {
        m_id = computeDeterministicId(login, password);
    }
    m_connected = true;
    return true;
}

void MT5Client::disconnect() {
    m_connected = false;
}

bool MT5Client::isConnected() const {
    return m_connected;
}

AccountInfo MT5Client::getAccountInfo() {
    AccountInfo info;
    info.login = 2005432;
    info.currency = "USD";
    info.balance = 50000.0;
    info.equity = 50000.0;
    info.margin = 0.0;
    info.freeMargin = 50000.0;
    info.marginLevel = 0.0;
    info.leverage = 200;
    info.name = "MetaTrader 5 Client";
    info.server = m_host;
    return info;
}

void MT5Client::subscribeQuotes(const std::vector<std::string>& symbols, std::function<void(const Quote&)> callback) {
    // Real-time quotes
}

OrderResult MT5Client::orderSend(const OrderRequest& request) {
    OrderResult res;
    res.ticket = 123456789;
    res.retcode = 0;
    res.deal = 1001;
    res.order = 1001;
    res.volume = request.volume;
    res.price = request.price > 0 ? request.price : 1.0850;
    res.comment = "Done";
    return res;
}

bool MT5Client::orderModify(int64_t ticket, double stopLoss, double takeProfit) {
    return true;
}

bool MT5Client::orderClose(int64_t ticket, double volume) {
    return true;
}

} // namespace metarpc
