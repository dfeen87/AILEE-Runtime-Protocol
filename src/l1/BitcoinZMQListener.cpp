#include "BitcoinZMQListener.h"
#include "../l2/ailee_sidechain_bridge.h"

namespace ailee {

#if defined(AILEE_HAS_ZMQ)

static uint64_t readLe64(const uint8_t* ptr) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(ptr[i]) << (i * 8));
    }
    return val;
}

static uint64_t readVarInt(const uint8_t*& ptr, const uint8_t* end) {
    if (ptr >= end) return 0;
    uint8_t first = *ptr++;
    if (first < 0xfd) return first;

    if (first == 0xfd) {
        if (ptr + 2 > end) return 0;
        uint64_t val = ptr[0] | (ptr[1] << 8);
        ptr += 2;
        return val;
    }
    if (first == 0xfe) {
        if (ptr + 4 > end) return 0;
        uint64_t val = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
        ptr += 4;
        return val;
    }
    if (ptr + 8 > end) return 0;
    uint64_t val = readLe64(ptr);
    ptr += 8;
    return val;
}

void BitcoinZMQListener::checkPegIn(const zmq::message_t& payload) {
    const uint8_t* ptr = static_cast<const uint8_t*>(payload.data());
    const uint8_t* end = ptr + payload.size();

    if (ptr + 4 > end) {
        std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
        return;
    }

    // read version (4 bytes)
    ptr += 4;

    bool isSegWit = false;
    if (ptr + 2 <= end && ptr[0] == 0x00 && ptr[1] == 0x01) {
        isSegWit = true;
        ptr += 2;
    }

    uint64_t inCount = readVarInt(ptr, end);
    if (inCount == 0 && !isSegWit) {
        std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
        return;
    }

    for (uint64_t i = 0; i < inCount; ++i) {
        if (ptr + 36 > end) {
            std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
            return;
        }
        ptr += 36; // outpoint
        uint64_t scriptLen = readVarInt(ptr, end);
        if (ptr + scriptLen > end) {
            std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
            return;
        }
        ptr += scriptLen;
        if (ptr + 4 > end) {
            std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
            return;
        }
        ptr += 4; // sequence
    }

    uint64_t outCount = readVarInt(ptr, end);
    bool bridgeFound = false;

    for (uint64_t i = 0; i < outCount; ++i) {
        if (ptr + 8 > end) {
            std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
            return;
        }
        uint64_t amount = readLe64(ptr);
        ptr += 8;

        uint64_t scriptLen = readVarInt(ptr, end);
        if (ptr + scriptLen > end) {
            std::cerr << "[ZMQ] Malformed transaction payload detected." << std::endl;
            return;
        }

        std::vector<uint8_t> scriptPubKey(ptr, ptr + scriptLen);
        ptr += scriptLen;

        if (!bridgeAddressBytes_.empty() && scriptPubKey == bridgeAddressBytes_) {
            bridgeFound = true;
            // Generate deterministic mock txid from payload. In a real scenario, this would double-SHA256 the serialized tx.
            // But since this is a deterministic system, we will just use the payload hash.

            std::vector<uint8_t> txidData; const uint8_t* p = static_cast<const uint8_t*>(payload.data()); size_t payloadSize = payload.size(); if (isSegWit) { txidData.insert(txidData.end(), p, p + 4); p += 6; txidData.insert(txidData.end(), p, ptr); const uint8_t* locktime_ptr = static_cast<const uint8_t*>(payload.data()) + payloadSize - 4; txidData.insert(txidData.end(), locktime_ptr, locktime_ptr + 4); } else { txidData.assign(p, p + payloadSize); }
            if (bridge_) {
                // Determine txid for real
                std::vector<uint8_t> hash1(SHA256_DIGEST_LENGTH);
                SHA256(txidData.data(), txidData.size(), hash1.data());
                std::vector<uint8_t> hash2(SHA256_DIGEST_LENGTH);
                SHA256(hash1.data(), hash1.size(), hash2.data());

                // reverse to match block explorer convention
                std::string txidHex = "";
                for (int j = SHA256_DIGEST_LENGTH - 1; j >= 0; --j) {
                    char buf[3];
                    snprintf(buf, sizeof(buf), "%02x", hash2[j]);
                    txidHex += buf;
                }

                std::cout << "[ZMQ] Valid peg-in detected: " << txidHex << " vout: " << i << " amount: " << amount << std::endl;

                // "Call into the appropriate L2/bridge handler to record the peg-in"
                bridge_->initiatePegIn(txidHex, static_cast<uint32_t>(i), amount, "unknown_source", "unknown_dest");
            }
        }
    }

    if (!bridgeFound) {
        std::cout << "[ZMQ] Non-bridge transaction skipped." << std::endl;
    }
}

#endif

} // namespace ailee
