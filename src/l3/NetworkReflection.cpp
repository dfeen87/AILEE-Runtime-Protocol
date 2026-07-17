#include "l3/NetworkReflection.h"
#include <json/json.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace ailee {
namespace l3 {

namespace {

void hexToBytes(const std::string& hex, uint8_t* out, size_t outLen) {
    std::memset(out, 0, outLen);
    if (hex.empty()) return;

    // Handle optional "0x" prefix
    size_t startIdx = (hex.length() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) ? 2 : 0;

    size_t hexLen = hex.length() - startIdx;


    // Copy bytes (right aligned if too short, truncated if too long)
    for (size_t i = 0; i < outLen && i * 2 < hexLen; ++i) {
        size_t hexPos = hex.length() - 1 - (i * 2);
        char c2 = hex[hexPos];
        char c1 = (hexPos > startIdx) ? hex[hexPos - 1] : '0';

        auto hexCharToInt = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
            return 0;
        };

        out[outLen - 1 - i] = (hexCharToInt(c1) << 4) | hexCharToInt(c2);
    }
}

} // anonymous namespace

NetworkBlockSnapshot parse_network_block_snapshot(const std::string& json_data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream s(json_data);
    if (!Json::parseFromStream(builder, s, &root, &errs)) {
        throw std::runtime_error("Failed to parse block snapshot JSON: " + errs);
    }

    NetworkBlockSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    if (root.isMember("hash")) {
        hexToBytes(root["hash"].asString(), snap.block_hash, 32);
    }
    if (root.isMember("previousblockhash")) {
        hexToBytes(root["previousblockhash"].asString(), snap.previous_block_hash, 32);
    }
    if (root.isMember("merkleroot")) {
        hexToBytes(root["merkleroot"].asString(), snap.merkle_root, 32);
    }

    snap.height = root.isMember("height") ? root["height"].asUInt64() : 0;
    snap.timestamp = root.isMember("time") ? root["time"].asUInt64() : 0;
    snap.nonce = root.isMember("nonce") ? root["nonce"].asUInt64() : 0;

    if (root.isMember("difficulty")) {
        // difficulty could be a float, we store an approximation or truncated int
        // Wait, difficulty_target is usually bits in uint32_t for deterministic representation.
        // Let's check if 'bits' is present, it's safer.
        if (root.isMember("bits")) {
            std::string bits_hex = root["bits"].asString();
            uint32_t bits = 0;
            try {
                bits = static_cast<uint32_t>(std::stoul(bits_hex, nullptr, 16));
            } catch (...) {}
            snap.difficulty_target = bits;
        } else {
            snap.difficulty_target = static_cast<uint32_t>(root["difficulty"].asDouble());
        }
    }

    snap.transaction_count = root.isMember("nTx") ? root["nTx"].asUInt() : 0;

    return snap;
}

NetworkMempoolSnapshot parse_network_mempool_snapshot(const std::string& json_data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream s(json_data);
    if (!Json::parseFromStream(builder, s, &root, &errs)) {
        throw std::runtime_error("Failed to parse mempool snapshot JSON: " + errs);
    }

    NetworkMempoolSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    snap.transaction_count = root.isMember("size") ? root["size"].asUInt64() : 0;
    snap.total_vsize = root.isMember("bytes") ? root["bytes"].asUInt64() : 0;

    // Total fees might not be directly in mempoolinfo in sats, but we use what we can or defaults
    if (root.isMember("total_fee")) {
        // usually in BTC as float, convert to sats
        snap.total_fee_sats = static_cast<uint64_t>(root["total_fee"].asDouble() * 100000000.0);
    }

    if (root.isMember("mempoolminfee")) {
        snap.min_fee_rate = static_cast<uint32_t>(root["mempoolminfee"].asDouble() * 100000000.0);
    }

    if (root.isMember("minrelaytxfee")) {
        snap.max_fee_rate = static_cast<uint32_t>(root["minrelaytxfee"].asDouble() * 100000000.0);
    }

    return snap;
}

NetworkPeerSnapshot parse_network_peer_snapshot(const std::string& json_data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream s(json_data);
    if (!Json::parseFromStream(builder, s, &root, &errs)) {
        throw std::runtime_error("Failed to parse peer snapshot JSON: " + errs);
    }

    NetworkPeerSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    if (root.isArray()) {
        snap.peer_count = root.size();
        snap.active_connections = 0;

        for (const auto& peer : root) {
            if (peer.isMember("bytesrecv")) {
                snap.total_bytes_recv += peer["bytesrecv"].asUInt64();
            }
            if (peer.isMember("bytessent")) {
                snap.total_bytes_sent += peer["bytessent"].asUInt64();
            }
            if (peer.isMember("connection_type")) {
                snap.active_connections++;
            }
        }
    }

    return snap;
}

} // namespace l3
} // namespace ailee
