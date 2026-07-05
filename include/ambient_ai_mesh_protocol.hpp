#ifndef AMBIENT_AI_MESH_PROTOCOL_HPP
#define AMBIENT_AI_MESH_PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <array>

namespace ailee {
namespace ambient_mesh {

using Hash256 = std::array<uint8_t, 32>;

// Canonical limits ensuring low-power device suitability and strict determinism.
struct AmbientMeshLimits {
    static constexpr uint32_t MAX_PAYLOAD_SIZE_BYTES = 512;
    static constexpr uint8_t MAX_HOP_COUNT = 5;
    static constexpr uint8_t MAX_FAN_OUT_DEGREE = 3;
    static constexpr uint32_t MAX_MESSAGES_PER_SECOND = 10;
};

enum class MeshMessageType : uint8_t {
    MICRO_MESSAGE = 0,
    CONTROL_MESSAGE = 1,
    PARTICIPATION_SUMMARY = 2
};

struct MeshMessageHeader {
    Hash256 messageId;             // Deterministic hash of payload + origin
    std::string originPeerId;      // Original sender libp2p Peer ID
    uint64_t logicalTimestamp;     // Protocol-defined logical tick
    uint8_t hopCount;              // Current hop count, strictly bounded by MAX_HOP_COUNT
    MeshMessageType type;

    // Binary serialization for deterministic alignment
    std::vector<uint8_t> serialize() const;
};

struct AmbientMeshMessage {
    MeshMessageHeader header;
    std::vector<uint8_t> payload;  // Strictly <= MAX_PAYLOAD_SIZE_BYTES
    std::vector<uint8_t> signature; // Origin ECDSA signature over the serialized header and payload

    // Uses secp256k1 equivalent wrapped logic
    bool verifySignature() const;
    Hash256 hash() const;
};

// Represents the active semantic link on top of a libp2p connection
struct MeshLink {
    uint64_t establishedTick;      // Logical tick when link was formed
    uint64_t lastActiveTick;       // Logical tick of last observed traffic
    bool isActive;
};

// Represents a deterministically tracked neighbor within the semantic mesh
struct NeighborEntry {
    std::string peerId;
    int64_t cachedEnergyScore;     // Derived from EnergyProfile
    MeshLink linkState;

    // Canonical Routing Priority Rule:
    // Primary ordering: higher, more stable energy scores first.
    // Tie-breaker: lexicographical PeerID sorting as a strict fallback.
    bool operator<(const NeighborEntry& other) const {
        if (cachedEnergyScore != other.cachedEnergyScore) {
            return cachedEnergyScore > other.cachedEnergyScore;
        }
        return peerId < other.peerId;
    }
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_PROTOCOL_HPP
