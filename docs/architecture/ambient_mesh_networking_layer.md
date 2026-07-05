# Ambient Mesh Networking Layer for AILEE CORE
## Protocol-Grade Architectural Specification

This document defines the deterministic Ambient Mesh Networking Layer that enables Ambient-AI nodes to discover peers, exchange micro-messages, and form a lightweight mesh overlay on top of AILEE CORE’s Bitcoin L2 coordination layer. The mesh relies on Bitcoin as a global coordination "energy source" and strictly adheres to deterministic, bounded, and low-power principles.

This specification seamlessly integrates with `bitcoin_anchoring_layer_spec.md`, `ambient_ai_primitives_spec.md`, and `ambient_node_mode_design.md`.

---

## 1. Deterministic Routing Algorithms (Low-Power Suitable)

Routing is strictly constrained by canonical limits. To maintain the mesh without randomness, routing prioritizes paths via nodes with high `EnergyProfile` scores, failing over deterministically to lexicographical `PeerID`s.

```cpp
#ifndef AMBIENT_AI_MESH_ROUTING_ENGINE_HPP
#define AMBIENT_AI_MESH_ROUTING_ENGINE_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <set>

// Forward declarations to allow compiling out-of-order architecture files
namespace ailee {
namespace ambient_mesh {
struct AmbientMeshMessage;
struct NeighborEntry;
}
}

namespace ailee {
namespace ambient_mesh {

class AmbientMeshRoutingEngine {
public:
    // Selects the next hop peers deterministically.
    // Fan-out is strictly bounded by AmbientMeshLimits::MAX_FAN_OUT_DEGREE.
    // Priority is based on NeighborEntry's operator< (energy score, then peerId).
    std::vector<std::string> selectNextHops(
        const AmbientMeshMessage& message,
        const std::set<NeighborEntry>& activeNeighbors
    ) const;

    // Evaluates if the local node can accept or forward a message.
    // Deterministically drops messages exceeding MAX_HOP_COUNT, MAX_PAYLOAD_SIZE_BYTES,
    // or exceeding the MAX_MESSAGES_PER_SECOND rate limit.
    bool validateRouteConstraints(
        const AmbientMeshMessage& message,
        uint32_t currentMessagesPerSecond
    ) const;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_ROUTING_ENGINE_HPP
```

---

## 2. Mesh Overlay Protocol Definition

The mesh overlay piggybacks on existing `libp2p` connections and `Gossipsub` topic memberships. It does not reinvent transport-level discovery, but rather acts as a deterministic semantic layer enforcing strictly bounded resource limits.

```cpp
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
```

---

## 3. Local Neighborhood State Management

Nodes maintain their local neighborhood strictly by observing libp2p traffic and piggybacking on `Gossipsub`. Caches and tables are deterministically pruned using logical ticks rather than wall-clock time.

```cpp
#ifndef AMBIENT_AI_MESH_NEIGHBORHOOD_HPP
#define AMBIENT_AI_MESH_NEIGHBORHOOD_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <array>
#include "ambient_ai_mesh_protocol.hpp" // Includes NeighborEntry and Hash256

namespace ailee {
namespace ambient_mesh {

struct RecentRouteCacheEntry {
    Hash256 messageId;
    uint64_t receivedTick;
};

class LocalNeighborhoodManager {
public:
    // Adds or updates a neighbor based on observed libp2p peer exchange and energy profile.
    void updateNeighborState(
        const std::string& peerId,
        int64_t energyScore,
        uint64_t currentLogicalTick
    );

    // Enforces deterministic eviction policies based on active link state and inactivity.
    // Avoids oscillation by requiring a fixed minimum logical tick threshold before eviction.
    void pruneInactiveNeighbors(uint64_t currentLogicalTick);

    // Retrieves the deterministically sorted active neighbors.
    std::set<NeighborEntry> getActiveNeighbors() const;

    // Tracks seen messages to prevent routing loops and broadcast storms.
    // Bounded cache; entries older than a fixed tick window are evicted deterministically.
    bool observeAndCacheMessage(const Hash256& messageId, uint64_t currentLogicalTick);

private:
    std::set<NeighborEntry> neighborTable;

    // Hash string serialization to CacheEntry. std::map guarantees deterministic iteration
    // and pruning order over messageId hashes.
    std::map<std::string, RecentRouteCacheEntry> recentRouteCache;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_NEIGHBORHOOD_HPP
```

---

## 4. Mesh Event Summarization and Bitcoin L2 Anchoring

Mesh activity generated by routing and participating is locally aggregated into an `AmbientMeshParticipationSummary`. At the epoch boundary, this summary feeds directly into the existing `AmbientParticipationRecord` defined in `ambient_node_mode_design.md`.

```cpp
#ifndef AMBIENT_AI_MESH_PARTICIPATION_HPP
#define AMBIENT_AI_MESH_PARTICIPATION_HPP

#include <cstdint>
#include <vector>

namespace ailee {
namespace ambient_mesh {

// Summarizes deterministic mesh routing and participation statistics
struct AmbientMeshParticipationSummary {
    uint32_t successfulRoutes;
    uint32_t failedRoutes;
    uint64_t meshUptimeSegments;

    // Serializes strictly into length-prefixed binary form
    std::vector<uint8_t> serialize() const;
};

class MeshParticipationAggregator {
public:
    // Increment deterministic counters triggered by AmbientMeshRoutingEngine
    void recordRoute(bool success);
    void recordUptimeSegment();

    // Finalizes the summary at epoch boundary.
    // This summary directly merges into ailee::ambient_node::AmbientParticipationRecord.
    // This consolidated record then influences the EnergyProfile and L2 Anchoring commitments.
    AmbientMeshParticipationSummary finalizeEpochSummary() const;

    // Resets internal counters at the beginning of a new epoch.
    void resetForNewEpoch();

private:
    AmbientMeshParticipationSummary currentSummary;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_PARTICIPATION_HPP
```

---

## 5. Integration with Existing P2P Modules

The Ambient Mesh operates transparently over `rust-libp2p` using a bridge pattern. It defines configuration flags injected via standard L2 bootstrap parameters and adds dedicated topics without modifying AILEE CORE consensus logic.

```cpp
#ifndef AMBIENT_AI_MESH_P2P_INTEGRATION_HPP
#define AMBIENT_AI_MESH_P2P_INTEGRATION_HPP

#include <cstdint>
#include <vector>
#include <string>
#include "ambient_ai_mesh_protocol.hpp" // Includes AmbientMeshLimits

namespace ailee {
namespace ambient_mesh {

// Injected configuration strictly controlling mesh behavior and limits
struct AmbientMeshConfig {
    bool ambientMeshEnabled = false; // Opt-in feature
    uint32_t maxMessagesPerSecond = AmbientMeshLimits::MAX_MESSAGES_PER_SECOND;
    uint32_t maxPayloadSizeBytes = AmbientMeshLimits::MAX_PAYLOAD_SIZE_BYTES;
};

// Decouples the deterministic semantic mesh from the underlying networking transport.
class IMeshP2PBridge {
public:
    virtual ~IMeshP2PBridge() = default;

    // Registers the deterministic mesh topic (e.g., "ailee/l2/ambient_mesh") via Gossipsub.
    virtual void registerTopics() = 0;

    // Broadcasts an AmbientMeshMessage directly to the topic.
    virtual void broadcast(const std::string& topic, const std::vector<uint8_t>& binaryPayload) = 0;

    // Callback invoked by the underlying P2P transport when a message is received on the topic.
    virtual void onMessageReceived(const std::string& peerId, const std::vector<uint8_t>& binaryPayload) = 0;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_P2P_INTEGRATION_HPP
```

---

## 6. Full Execution Plan

### Phase 1: Protocol Structs & Local Neighborhood State
1. **Implementation:**
   - Define `AmbientMeshLimits`, `AmbientMeshMessage`, `NeighborEntry`.
   - Implement `LocalNeighborhoodManager` with logical tick-based caching and deterministic eviction.
2. **Deterministic Success Criteria:**
   - Test cases assert that `NeighborEntry` strictly orders by `cachedEnergyScore`, falling back to `peerId` upon ties.
   - Assert `observeAndCacheMessage` guarantees idempotency without randomness.

### Phase 2: Deterministic Routing Engine
1. **Implementation:**
   - Implement `AmbientMeshRoutingEngine`.
   - Enforce explicit `AmbientMeshLimits` checking before any message processing.
2. **Deterministic Success Criteria:**
   - Test under heavy adversarial spam (exceeding `MAX_MESSAGES_PER_SECOND` limit) guarantees deterministic hard drops without side-effects.
   - Assert `selectNextHops` strictly selects the highest energy peers, bounded by `MAX_FAN_OUT_DEGREE`.

### Phase 3: Mesh Event Summarization & Integration
1. **Implementation:**
   - Build `MeshParticipationAggregator`.
   - Plumb `AmbientMeshParticipationSummary` directly into `AmbientParticipationRecord` in `src/ambient_node/`.
2. **Deterministic Success Criteria:**
   - Guarantee the resulting `AmbientParticipationRecord` serialization retains byte-for-byte correctness matching previous epoch states, now factoring in mesh events.
   - Assert `EnergyProfile` shifts exactly according to scaled route metrics.

### Phase 4: Final P2P Bridging & Validation
1. **Implementation:**
   - Instantiate `IMeshP2PBridge` wrapping `rust-libp2p` FFI callbacks.
   - Map `ambientMeshEnabled` in core launch scripts.
2. **Testing & Resource Usage Validation:**
   - Profile CPU/Memory locally: Assert operations remain purely deterministic and memory limits do not exceed bounds under sustained 10 messages/sec load.
   - Test partial connectivity scenarios: Assert deterministic fallback routing to lower-energy nodes works appropriately without loops.
   - Test Sybil flooding: Assert that `verifySignature` combined with the deterministic reputation cache rejects duplicate identities and payload collisions seamlessly.
