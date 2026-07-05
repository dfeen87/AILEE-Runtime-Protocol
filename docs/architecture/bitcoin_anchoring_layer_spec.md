# Ambient AI L2 Foundational Bitcoin Anchoring Layer
## Protocol-Grade Architectural Specification

This document defines the deterministic C++ layer required to anchor the Ambient AI Internet L2 network into Bitcoin. It serves as the definitive specification for protocol layout, data structures, invariants, and ownership lifetimes.

---

## 1. Core Epoch Structure

The highest-level construct of the L2 anchoring mechanism is the Epoch. Nodes accumulate state, compute energy, and execute L2 logic over an epoch, culminating in a deterministic Taproot commitment anchored to Bitcoin.

```cpp
#ifndef AMBIENT_AI_EPOCH_HPP
#define AMBIENT_AI_EPOCH_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace ailee {
namespace protocol {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic constants for energy calculus (Scale = 10000)
constexpr int64_t FIXED_POINT_SCALE = 10000;
constexpr int64_t ALPHA_COMPUTE = 6000; // 0.6
constexpr int64_t BETA_UPTIME   = 3000; // 0.3
constexpr int64_t GAMMA_STORAGE = 1000; // 0.1

struct AmbientAIAnchoringEpoch {
    uint64_t epochId;               // Monotonically increasing L2 epoch counter
    uint64_t startBitcoinHeight;    // L1 block height where epoch begins
    uint64_t endBitcoinHeight;      // L1 block height where epoch ends (inclusive)

    Hash256 l2StateRoot;            // Deterministic state root of the L2 Ledger, Bridge, and Orchestrator
    Hash256 energyLedgerRoot;       // Merkle root of all node energy balances (See ambient_ai_primitives_spec.md)
    Hash256 participationRoot;      // Merkle root of deterministic node identity/participation proofs (See ambient_ai_primitives_spec.md)
    Hash256 ambientEventRoot;       // Merkle root of the Ambient Internet extension event tree (See ambient_ai_primitives_spec.md)

    Hash256 protocolBuildMetadata;  // Hash of the deterministic BuildInfo (commit, version)

    // The final payload to be serialized and committed into the Taproot tree.
    // The resulting commitment forms a single TapLeaf within the TapTree.
    Hash256 computeEpochCommitment() const;
};

} // namespace protocol
} // namespace ailee

#endif // AMBIENT_AI_EPOCH_HPP
```

### Invariants:
* `epochId` must strictly increment by 1 for every subsequent epoch.
* `endBitcoinHeight - startBitcoinHeight` defines the epoch length `N` (e.g., 144 blocks).
* All internal hashes must use exact SHA-256 byte-level implementations matching `libsecp256k1`.
* Identity, energy, and events are strictly merkleized under this epoch root, which then acts as the solitary Taproot commitment (TapLeaf).
* Detailed structures, formulas, and mesh propagation rules for the constituent roots are strictly defined in `ambient_ai_primitives_spec.md`.

---

## 2. Bitcoin Full Node View (Deterministic Connector)

The layer connects to a full Bitcoin Core node via JSON-RPC (for historical queries/actions) and ZMQ (for real-time deterministic streaming of blocks and transactions).

```cpp
#ifndef AMBIENT_AI_BITCOIN_NODE_VIEW_HPP
#define AMBIENT_AI_BITCOIN_NODE_VIEW_HPP

#include <vector>
#include <map>
#include <memory>

namespace ailee {
namespace bitcoin {

using Hash256 = std::array<uint8_t, 32>;

// Canonical representation of a validated Bitcoin block header
struct BlockHeader {
    int32_t version;
    Hash256 prevBlockHash;
    Hash256 merkleRoot;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
    Hash256 blockHash; // Cached self-hash (double SHA-256)
};

// Represents a deterministic, raw transaction view
struct RawTransaction {
    Hash256 txId;
    std::vector<uint8_t> serializedBytes;
    bool hasWitness;
};

// Represents the deterministically verified Merkle Proof of inclusion
struct MerkleProof {
    Hash256 txId;
    Hash256 blockHash;
    std::vector<Hash256> siblingHashes;
    uint32_t txIndex;

    // Validates the proof against a known block header Merkle root
    bool verify(const Hash256& expectedMerkleRoot) const;
};

// Abstract interface for the deterministic L1 View
class IBitcoinNodeView {
public:
    virtual ~IBitcoinNodeView() = default;

    // Returns the current confirmed tip of the Bitcoin chain
    virtual BlockHeader getChainTip() const = 0;

    // Fetches a specific header deterministically. Throws if missing.
    virtual BlockHeader getHeader(const Hash256& blockHash) const = 0;
    virtual BlockHeader getHeaderByHeight(uint64_t height) const = 0;

    // Checks if an epoch anchor commitment exists within a specific block
    virtual bool containsTaprootCommitment(
        const Hash256& blockHash,
        const Hash256& expectedCommitment
    ) const = 0;

    // Stream subscriptions (ZMQ backed)
    // Implementations must strictly sequence events.
    class IBlockSubscriber {
    public:
        virtual ~IBlockSubscriber() = default;
        virtual void onBlockConnected(const BlockHeader& header, const std::vector<RawTransaction>& txs) = 0;
        virtual void onBlockDisconnected(const BlockHeader& header) = 0; // Reorg handling
    };

    virtual void subscribe(std::shared_ptr<IBlockSubscriber> subscriber) = 0;
};

} // namespace bitcoin
} // namespace ailee

#endif // AMBIENT_AI_BITCOIN_NODE_VIEW_HPP
```

### Invariants:
* `onBlockConnected` and `onBlockDisconnected` must be executed synchronously and deterministically on the main consensus thread to guarantee deterministic state replay.
* No asynchronous threading model may intercept ZMQ events without pushing them to a deterministic logical queue driven by the L2 epoch engine.

---

## 3. Taproot Anchoring Layout

Anchors commit the `AmbientAIAnchoringEpoch` directly to a Bitcoin Taproot tree (TapTree).

```cpp
#ifndef AMBIENT_AI_TAPROOT_ANCHOR_HPP
#define AMBIENT_AI_TAPROOT_ANCHOR_HPP

#include <vector>

namespace ailee {
namespace anchoring {

// Uses libsecp256k1 for all taproot tweak computations
class TaprootAnchor {
public:
    // Internal federation public key (Aggregate key)
    std::array<uint8_t, 32> internalPubKey;

    // Builds the TapTree leaves
    // Leaf 0: L2 Epoch Commitment
    // Leaf 1: Optional BitVM-style challenge response script
    struct TapTreeState {
        std::vector<uint8_t> controlBlock;
        std::array<uint8_t, 32> tweakedPubKey;
        std::vector<uint8_t> script; // OP_RETURN or OP_FALSE OP_IF ... OP_ENDIF
    };

    // Computes the exact Taproot tweak needed to commit the epoch
    TapTreeState buildEpochCommitment(
        const protocol::AmbientAIAnchoringEpoch& epoch
    ) const;

    // Verifies an existing Taproot output contains our commitment
    bool verifyCommitment(
        const std::array<uint8_t, 32>& outputPubKey,
        const protocol::AmbientAIAnchoringEpoch& expectedEpoch
    ) const;
};

} // namespace anchoring
} // namespace ailee

#endif // AMBIENT_AI_TAPROOT_ANCHOR_HPP
```

### Invariants:
* The commitment is placed in a Tapscript leaf formatted as: `OP_FALSE OP_IF <32_BYTE_EPOCH_COMMITMENT> OP_ENDIF`.
* The `internalPubKey` must be the static configuration operator key (NUMS points are strictly prohibited).

---

## 4. Energy Abstraction for Nodes

Energy dictates consensus weight and reward distributions. Energy relies on fixed-point arithmetic to maintain cross-platform determinism.
**Note:** This section provides the foundational anchor; full derivation rules, bounding logic, and mesh network routing hints are strictly defined in `ambient_ai_primitives_spec.md`.

```cpp
#ifndef AMBIENT_AI_ENERGY_MODEL_HPP
#define AMBIENT_AI_ENERGY_MODEL_HPP

#include <cstdint>
#include <string>

namespace ailee {
namespace energy {

struct EnergyProfile {
    std::string nodeId; // libp2p PeerId

    // Raw unscaled metrics accumulated during the epoch
    uint64_t computeCompletedJobs;
    uint64_t uptimeMilliseconds;
    uint64_t storageBytesProvided;

    // Deterministically scaled and combined energy value (Scale = 10000)
    int64_t calculateEpochEnergy() const {
        // Use 128-bit ints for intermediate multiplication to prevent overflow
        __int128_t computeScore = static_cast<__int128_t>(computeCompletedJobs) * protocol::ALPHA_COMPUTE;
        __int128_t uptimeScore = static_cast<__int128_t>(uptimeMilliseconds) * protocol::BETA_UPTIME;
        __int128_t storageScore = static_cast<__int128_t>(storageBytesProvided) * protocol::GAMMA_STORAGE;

        __int128_t totalEnergyScaled = computeScore + uptimeScore + storageScore;

        // Downscale
        return static_cast<int64_t>(totalEnergyScaled / protocol::FIXED_POINT_SCALE);
    }

    // Serialization for Merkle tree insertion
    std::vector<uint8_t> canonicalSerialize() const;
};

} // namespace energy
} // namespace ailee

#endif // AMBIENT_AI_ENERGY_MODEL_HPP
```

### Invariants:
* All intermediate calculations must use `__int128_t`.
* Float or double types are strictly forbidden.
* Negative energy (penalties) decays bounded to 0 (no negative balances in the root ledger).

---

## 5. Deterministic Participation Proofs and Metadata

Nodes must prove they participated with the correctly compiled consensus version.
**Note:** This section provides the foundational anchor; full rigorous definitions of the identity model, secp256k1 canonical hashing, and structural bounds are strictly defined in `ambient_ai_primitives_spec.md`.

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_HPP
#define AMBIENT_AI_PARTICIPATION_HPP

#include <vector>
#include <string>

namespace ailee {
namespace identity {

struct BuildMetadata {
    std::string commitHash;
    uint32_t buildNumber;
    uint32_t protocolVersion;

    std::array<uint8_t, 32> hash() const;
};

struct ParticipationProof {
    std::string peerId;
    std::vector<uint8_t> nodeEcdsaSignature; // Sign( hash( epochId || BuildMetadata ) )
    BuildMetadata metadata;

    bool verify() const; // Validates signature and protocolVersion against consensus rules
};

} // namespace identity
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_HPP
```

### Invariants:
* `BuildMetadata` values must be directly populated from the CMake preprocessor definitions sourced from `src/build/BuildInfo.hpp`.
* Invalid participation proofs result in 0 energy awarded for the epoch.

---

## 6. L2 Mesh Prerequisites (rust-libp2p + Gossipsub)

The communication backbone dictates how epoch data is propagated.

```cpp
#ifndef AMBIENT_AI_MESH_PREREQUISITES_HPP
#define AMBIENT_AI_MESH_PREREQUISITES_HPP

namespace ailee {
namespace mesh {

// Canonical Topic Layout
constexpr char TOPIC_L2_STATE_DIFF[] = "ailee/l2/state_diff";
constexpr char TOPIC_ZK_PROOF[]      = "ailee/l2/zk_proof";
constexpr char TOPIC_TELEMETRY[]     = "ailee/ai/telemetry";
constexpr char TOPIC_EPOCH_ANCHOR[]  = "ailee/l2/epoch_anchor";

// Communication rules:
// 1. All payloads must be strictly binary length-prefixed format (JSON prohibited in mesh payloads).
// 2. Nodes validate the ECDSA signature of the sender via PeerId before routing.
// 3. Rate limiting is driven by `ReputationRateLimiter` using a monotonic logical clock, never wall-clock time.

class IMeshTransport {
public:
    virtual ~IMeshTransport() = default;

    // Broadcast a fully serialized epoch proposal or Diff
    virtual void publish(const std::string& topic, const std::vector<uint8_t>& binaryPayload) = 0;
};

} // namespace mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_PREREQUISITES_HPP
```

---

## 7. Ambient Internet Extension Preparation

Ambient events generated by IoT or external devices are aggregated into a Merkle tree and anchored alongside the L2 state.
**Note:** This section provides the foundational anchor; full rigorous definitions, deterministic hashing, bounded limits, and Data Availability (DA) network integration are strictly defined in `ambient_ai_primitives_spec.md`.

```cpp
#ifndef AMBIENT_AI_INTERNET_EXTENSION_HPP
#define AMBIENT_AI_INTERNET_EXTENSION_HPP

#include <vector>
#include <array>

namespace ailee {
namespace ambient {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientEvent {
    Hash256 eventId;
    std::string deviceId;
    uint64_t logicalTimestamp;
    std::vector<uint8_t> payload; // Bounded to 1024 bytes per protocol rules

    // Cryptographically binds the payload to the device
    std::vector<uint8_t> deviceSignature;

    bool verifySignature() const;
    Hash256 hash() const;
};

class AmbientEventAggregator {
public:
    // Pushes an event into the current epoch's memory pool
    bool submitEvent(const AmbientEvent& event);

    // Deterministically sorts and hashes all events to produce the ambientEventRoot
    // Conflict resolution: If duplicate eventIds exist, the one with the lowest
    // lexicographical deviceSignature is retained.
    Hash256 finalizeEpochRoot() const;
};

} // namespace ambient
} // namespace ailee

#endif // AMBIENT_AI_INTERNET_EXTENSION_HPP
```

### Invariants:
* `payload` size must strictly be `<= 1024` bytes.
* `logicalTimestamp` is a protocol-defined epoch sub-tick, not system wall-clock time.
* `ambientEventRoot` is injected into `AmbientAIAnchoringEpoch::ambientEventRoot`.
