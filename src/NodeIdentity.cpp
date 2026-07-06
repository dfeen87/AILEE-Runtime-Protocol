#include "NodeIdentity.h"
#include "BuildInfo.h"
#include "L1Reflection.h" // contains GenesisInfo
#include "ConfigInfo.h"

#include <cstring>
#include <openssl/sha.h>

namespace ailee {
namespace identity {

// Helper to compute SHA-256 using OpenSSL
static void compute_sha256(const uint8_t* data, size_t len, uint8_t out_hash[32]) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, len);
    SHA256_Final(out_hash, &sha256);
}

NodeId compute_node_id(
    const BuildInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
) {
    // Buffer layout (in order):
    // - build.build_hash[32]
    // - build.build_number (uint32_t, little-endian)
    // - genesis.genesis_anchor_root[32]
    // - config.config_hash[32]
    // - config.protocol_version (uint32_t, little-endian)

    constexpr size_t BUFFER_SIZE = 32 + 4 + 32 + 32 + 4;
    uint8_t buffer[BUFFER_SIZE];
    size_t offset = 0;

    // build.build_hash[32]
    std::memcpy(buffer + offset, build.build_hash, 32);
    offset += 32;

    // build.build_number (uint32_t, little-endian)
    // Ensure little-endian serialization
    buffer[offset++] = static_cast<uint8_t>(build.build_number & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((build.build_number >> 8) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((build.build_number >> 16) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((build.build_number >> 24) & 0xFF);

    // genesis.genesis_anchor_root[32]
    std::memcpy(buffer + offset, genesis.genesis_anchor_root, 32);
    offset += 32;

    // config.config_hash[32]
    std::memcpy(buffer + offset, config.config_hash, 32);
    offset += 32;

    // config.protocol_version (uint32_t, little-endian)
    buffer[offset++] = static_cast<uint8_t>(config.protocol_version & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((config.protocol_version >> 8) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((config.protocol_version >> 16) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((config.protocol_version >> 24) & 0xFF);

    NodeId result;
    compute_sha256(buffer, BUFFER_SIZE, result.id);
    return result;
}

bool node_id_equal(const NodeId& a, const NodeId& b) {
    return std::memcmp(a.id, b.id, 32) == 0;
}

int node_id_compare_lex(const NodeId& a, const NodeId& b) {
    return std::memcmp(a.id, b.id, 32);
}

NodeId compute_node_id_for_mesh(
    const BuildInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
) {
    return compute_node_id(build, genesis, config);
}

} // namespace identity
} // namespace ailee
