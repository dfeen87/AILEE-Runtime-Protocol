# Ambient AI L2 Identity–Energy–Event Substrate
## Protocol-Grade Architectural Specification

This document defines the deterministic C++ substrate for the three remaining consensus-critical primitives required by the Ambient AI Internet L2 Extension: Node Identity, Node Energy, and Ambient Events.

This specification serves as the deep technical expansion of the roots established in `bitcoin_anchoring_layer_spec.md`. All primitives defined here are merkleized strictly beneath the single `AmbientAIAnchoringEpochRoot`, which in turn forms the singular TapLeaf commitment anchored to the Bitcoin L1.

---

## 1. Deterministic Node Identity Model

Node Identity binds physical hardware and protocol compilation state to a cryptographic identity. It ensures that network participants are executing deterministically verifiable, byte-for-byte identical builds of the AILEE Core.

### Compilation-Ready Architecture

```cpp
#ifndef AMBIENT_AI_NODE_IDENTITY_HPP
#define AMBIENT_AI_NODE_IDENTITY_HPP

#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace ailee {
namespace identity {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic metadata reflecting the node's compilation environment.
// These values are injected via CMake preprocessor macros from BuildInfo.hpp.
struct BuildMetadata {
    std::string commitHash;      // 40-character git commit hex string
    uint32_t buildNumber;        // Monotonically increasing build ID
    uint32_t protocolVersion;    // Strict protocol consensus version

    // Returns the deterministic SHA-256 hash of the serialized metadata.
    Hash256 canonicalHash() const;
};

// The core cryptographic identity payload that a node signs to participate.
struct IdentityPayload {
    std::string peerId;          // libp2p Peer ID (base58 or multihash)
    std::string publicKeyHex;    // secp256k1 public key in uncompressed hex format (130 chars)
    BuildMetadata metadata;      // The deterministic build signature
    uint64_t epochId;            // The epoch this identity is committing to
    uint64_t staticAttribute;    // Optional protocol-defined static attribute (e.g., node type)

    // Serializes the payload into a strictly deterministic byte array.
    std::vector<uint8_t> serialize() const;

    // Computes the SHA-256 hash over the serialized payload.
    // Must strictly use libsecp256k1 equivalent (e.g., via secp256k1_sha256).
    Hash256 hash() const;
};

// The participation proof submitted by the node to the mesh network.
struct ParticipationProof {
    IdentityPayload payload;
    std::vector<uint8_t> ecdsaSignature; // Strict DER-encoded secp256k1 signature over payload.hash()

    // Verifies the ECDSA signature against the payload's public key.
    // Also validates that protocolVersion matches the current consensus requirement.
    bool verify() const;
};

} // namespace identity
} // namespace ailee

#endif // AMBIENT_AI_NODE_IDENTITY_HPP
```

### Derivation & Subsystem Boundaries

* **Derivation:** The `IdentityPayload` binds the node's ephemeral or permanent `publicKeyHex` tightly to the `BuildMetadata`. Any deviation in the Git commit hash or protocol version instantly alters the identity payload hash, invalidating the node's consensus standing.
* **Canonical Hashing:** `IdentityPayload::hash()` MUST utilize a strict wrapper around `libsecp256k1` hashing (e.g., `secp256k1_sha256`) to ensure exact byte-compatibility with Bitcoin Taproot conventions. No alternative hashing implementations are permitted.
* **Subsystem Boundary (BuildInfo):** The `BuildMetadata` struct fields are statically populated at startup by querying `src/build/BuildInfo.hpp`.
* **Mesh Propagation:** `ParticipationProof` objects are broadcast over the L2 mesh (via rust-libp2p/Gossipsub) using a dedicated topic (e.g., `ailee/l2/identity`). They are aggregated in a local Mempool.
* **Epoch Binding:** At epoch boundary, the `BlockProducer` gathers all verified `ParticipationProof`s, deterministically sorts them by `publicKeyHex`, and hashes them into a Merkle tree to form the `participationRoot` stored within `AmbientAIAnchoringEpoch`.

---

## 2. Node Energy / Participation Model

Energy translates physical world effort (compute, uptime, storage) into deterministic consensus weight. Energy values modulate Gossipsub topic priority and L2 Liveness guarantees.

### Compilation-Ready Architecture

```cpp
#ifndef AMBIENT_AI_ENERGY_MODEL_HPP
#define AMBIENT_AI_ENERGY_MODEL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

namespace ailee {
namespace energy {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic constants for energy calculus (Scale = 10000)
constexpr int64_t FIXED_POINT_SCALE = 10000;
constexpr int64_t ALPHA_COMPUTE     = 6000; // 0.6
constexpr int64_t BETA_UPTIME       = 3000; // 0.3
constexpr int64_t GAMMA_STORAGE     = 1000; // 0.1

// Absolute hardware bounds to prevent artificial overflow exploits
constexpr uint64_t MAX_COMPUTE_JOBS     = 1000000;      // 1M jobs per epoch
constexpr uint64_t MAX_UPTIME_MS        = 86400000;     // 24 hours in ms
constexpr uint64_t MAX_STORAGE_BYTES    = 107374182400; // 100 GB

struct EnergyProfile {
    std::string publicKeyHex; // Binds directly to IdentityPayload::publicKeyHex

    // Raw unscaled metrics accumulated deterministically during the epoch
    uint64_t computeCompletedJobs;
    uint64_t uptimeMilliseconds;
    uint64_t storageBytesProvided;

    // Deterministically scaled and combined energy value (Scale = 10000)
    int64_t calculateEpochEnergy() const {
        // Enforce protocol upper bounds
        uint64_t boundedCompute = std::min(computeCompletedJobs, MAX_COMPUTE_JOBS);
        uint64_t boundedUptime = std::min(uptimeMilliseconds, MAX_UPTIME_MS);
        uint64_t boundedStorage = std::min(storageBytesProvided, MAX_STORAGE_BYTES);

        // Use 128-bit ints for intermediate multiplication to prevent overflow
        __int128_t computeScore = static_cast<__int128_t>(boundedCompute) * ALPHA_COMPUTE;
        __int128_t uptimeScore  = static_cast<__int128_t>(boundedUptime) * BETA_UPTIME;
        __int128_t storageScore = static_cast<__int128_t>(boundedStorage) * GAMMA_STORAGE;

        __int128_t totalEnergyScaled = computeScore + uptimeScore + storageScore;

        // Downscale
        return static_cast<int64_t>(totalEnergyScaled / FIXED_POINT_SCALE);
    }

    // Serializes strictly into length-prefixed binary form for Merkle leaf creation
    std::vector<uint8_t> canonicalSerialize() const;

    Hash256 hash() const;
};

// Represents a historical cumulative state of a node's energy.
struct EnergyLedgerEntry {
    std::string publicKeyHex;
    uint64_t cumulativeEnergy; // Accrues positive energy, floors at 0 for penalties

    // Applies a deterministic epoch-over-epoch decay to prevent perpetual dominance.
    // e.g., decays by 5% every epoch: newEnergy = (cumulativeEnergy * 9500) / FIXED_POINT_SCALE
    void applyEpochDecay() {
        constexpr int64_t DECAY_FACTOR = 9500; // 0.95 multiplier
        __int128_t decayed = static_cast<__int128_t>(cumulativeEnergy) * DECAY_FACTOR;
        cumulativeEnergy = static_cast<uint64_t>(decayed / FIXED_POINT_SCALE);
    }
};

} // namespace energy
} // namespace ailee

#endif // AMBIENT_AI_ENERGY_MODEL_HPP
```

### Derivation & Subsystem Boundaries

* **Derivation:** Energy is purely derived from on-chain (L2) provable actions: jobs proven via ZK-proofs (Compute), observed heartbeat connectivity proofs (Uptime), and Data Availability challenge responses (Storage).
* **Bounds & Fixed-Point Math:** All intermediate calculations must use `__int128_t`. All raw inputs are hard-capped using the `MAX_*` constants to ensure determinism across different architectures and prevent massive spam or overflow vectors. Floating-point logic is strictly prohibited.
* **Mesh Routing Hints:** Energy scores directly inject into `ReputationRateLimiter`. Nodes with higher derived `calculateEpochEnergy()` receive multiplier boosts on their gossipsub message forwarding rate limits, ensuring high-energy nodes dominate the peer priority list.
* **Epoch Binding:** The local orchestrator aggregates all verified `EnergyProfile` structs into an array, sorts them lexicographically by `publicKeyHex`, and hashes them to produce the `energyLedgerRoot` included in the `AmbientAIAnchoringEpoch`.

---

## 3. Ambient Event Commitment Format

Ambient Events represent physical world data (IoT sensors, L2 telemetrics, external triggers) injected into the L2. They are deterministically ordered and anchored into the protocol state.

### Compilation-Ready Architecture

```cpp
#ifndef AMBIENT_AI_EVENT_COMMITMENT_HPP
#define AMBIENT_AI_EVENT_COMMITMENT_HPP

#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace ailee {
namespace ambient {

using Hash256 = std::array<uint8_t, 32>;

constexpr size_t MAX_EVENT_PAYLOAD_SIZE = 1024; // 1 KB max

// Canonical structure for an external ambient event
struct AmbientEvent {
    Hash256 eventId;                 // Unique identifier generated by device
    std::string deviceId;            // Registered identity of the sensing device
    uint64_t logicalTimestamp;       // Protocol-defined logical tick (NOT wall-clock)
    std::vector<uint8_t> payload;    // Arbitrary binary data, <= 1024 bytes

    // Cryptographic binding over eventId || deviceId || logicalTimestamp || payload
    std::vector<uint8_t> deviceSignature;

    // Performs deterministic bounds checking and secp256k1 ECDSA verification
    bool verify() const;

    // Serializes and computes the SHA-256 hash (secp256k1 wrapper) of the event
    Hash256 hash() const;
};

// Aggregates events and interfaces with the Data Availability (DA) network
class AmbientEventAggregator {
public:
    // Pushes an event into the local pending queue.
    // Silently drops events failing verify() or exceeding MAX_EVENT_PAYLOAD_SIZE.
    bool submitEvent(const AmbientEvent& event);

    // Finalizes the epoch's event tree.
    // 1. Sorts all pending events deterministically by (logicalTimestamp, deviceId, eventId).
    // 2. Conflict resolution: If identical eventIds exist, retain the one with the lexicographically lowest deviceSignature.
    // 3. Produces a Merkle root from the sorted hashes.
    Hash256 finalizeEpochRoot() const;

    // Packages the raw events into an L2StateDiff equivalent structure for DA propagation.
    std::vector<uint8_t> exportToDAPayload() const;
};

} // namespace ambient
} // namespace ailee

#endif // AMBIENT_AI_EVENT_COMMITMENT_HPP
```

### Derivation & Subsystem Boundaries

* **Derivation:** Events are completely standalone payloads that must self-verify (`deviceSignature`). They depend on a `logicalTimestamp` aligned to the L2 Liveness sub-tick, completely decoupling them from the host machine's wall-clock time.
* **DA Integration:** While the Merkle root of these events (`ambientEventRoot`) is anchored into the `AmbientAIAnchoringEpoch`, the bulky raw event `payload` arrays are routed to the Data Availability network. `exportToDAPayload()` generates a chunked byte array that acts as an adjunct to `L2StateDiff` payloads on the mesh.
* **Mesh Propagation:** Raw events are propagated via libp2p + Gossipsub on a dedicated topic (`ailee/ai/ambient_events`). They are rigorously rate-limited and subjected to binary-only serialization. JSON parsing is strictly disallowed on the mesh transport path for events.
* **Epoch Binding:** The finalized root computed via `AmbientEventAggregator::finalizeEpochRoot()` becomes the `ambientEventRoot` field within the `AmbientAIAnchoringEpoch`.
