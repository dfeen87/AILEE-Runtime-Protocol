#pragma once

#include <cstdint>

namespace ailee {

// Forward declarations for structs containing deterministic inputs
struct BuildInfo;
struct GenesisInfo;
struct ConfigInfo;

namespace identity {

// 32-byte deterministic fingerprint (SHA-256)
// POD, trivially copyable, no std::string, no dynamic allocation
struct alignas(64) NodeId {
    uint8_t id[32];
};

// Compute deterministic NodeID from stable protocol inputs
NodeId compute_node_id(
    const BuildInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
);

// Identity comparison helpers
bool node_id_equal(const NodeId& a, const NodeId& b);
int node_id_compare_lex(const NodeId& a, const NodeId& b);

// Helper for bridging identity into mesh
NodeId compute_node_id_for_mesh(
    const BuildInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
);

} // namespace identity
} // namespace ailee
