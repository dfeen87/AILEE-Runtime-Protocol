# Ambient Participation Proofs for Bitcoin L2
## Protocol-Grade Architectural Specification

This document defines the deterministic "Ambient Participation Proof" system that enables Ambient-AI nodes to contribute sensing, routing, and uptime to the Ambient Mesh and anchor those contributions into AILEE CORE's Bitcoin L2 layer. This system establishes a deterministic, non-monetary "give-back" mechanism where ambient infrastructure supports the Bitcoin network through coordination and routing prioritization.

This specification seamlessly integrates with `bitcoin_anchoring_layer_spec.md`, `ambient_ai_primitives_spec.md`, and `ambient_mesh_networking_layer.md`.

---

## 1. Valid Ambient Contributions

Ambient nodes generate deterministic contributions through routing, sensing, and maintaining uptime. These contributions are discretized into strictly defined buckets to preserve privacy and bounded to ensure low-power suitability.

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP
#define AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace ailee {
namespace participation {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic categories of ambient contributions
enum class ContributionType : uint8_t {
    ROUTING_SUCCESS = 0,    // Successful micro-route propagation
    SENSING_SNAPSHOT = 1,   // Valid privacy-preserving signal snapshot
    UPTIME_SEGMENT = 2      // Verified low-power node availability segment
};

// Privacy-preserving discrete buckets for sensing events
struct AmbientContributionBucket {
    uint8_t powerStateClass;     // e.g., 0=Battery, 1=Charging, 2=Mains
    uint8_t connectivityBand;    // e.g., 0=LowBandwidth, 1=HighBandwidth
    uint8_t activityLevel;       // Deterministically discretized activity index

    // Serializes to a strictly bounded byte array
    std::vector<uint8_t> serialize() const;
};

// Represents a single, deterministic contribution event logged by a node
struct AmbientContributionEvent {
    Hash256 eventId;                 // Hash of local context (must not contain sensitive data)
    uint64_t logicalTimestamp;       // Protocol-defined logical tick (NOT wall-clock)
    ContributionType type;
    AmbientContributionBucket bucket;

    // Binary serialization for deterministic aggregation
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP
```

### Invariants:
* `logicalTimestamp` strictly aligns with the L2 epoch sub-ticks. Wall-clock time is prohibited.
* `AmbientContributionBucket` ensures raw telemetry data is never logged or exposed. Instead, data is abstracted into strict, deterministic enum-style bounds.

---

## 2. Deterministic Measurement and Anti-Cheating Rules

Contributions must be resistant to trivial gaming, replay attacks, and spam. Nodes maintain a deterministic log of events that are strictly bounded and summarized per epoch.

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP
#define AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP

#include <cstdint>
#include <vector>
#include <set>
#include <array>
#include "ambient_participation_contributions.hpp"

namespace ailee {
namespace participation {

// Canonical limits to prevent overflow and spam attacks
struct ParticipationLimits {
    static constexpr uint32_t MAX_EVENTS_PER_EPOCH = 5000;
    static constexpr uint32_t MAX_SENSING_PER_TICK = 1;
};

// Summarizes a full epoch's worth of contribution events deterministically
struct AmbientContributionSummary {
    uint32_t totalRoutingSuccesses;
    uint32_t totalSensingSnapshots;
    uint32_t totalUptimeSegments;

    // Serializes into a canonical length-prefixed binary array
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

// Deterministic event logger enforcing rate-limits and deduplication
class AmbientContributionLog {
public:
    // Attempts to log an event. Deterministically drops events that exceed:
    // - MAX_EVENTS_PER_EPOCH
    // - Duplicate eventId (replay protection)
    // - Rate limits (e.g., MAX_SENSING_PER_TICK)
    bool logEvent(const AmbientContributionEvent& event);

    // Finalizes the epoch's valid events into a bounded summary.
    // Clears the internal log state for the next epoch.
    AmbientContributionSummary finalizeEpochSummary();

private:
    std::vector<AmbientContributionEvent> validEvents;

    // Custom comparator for deterministic ordering of Hashes
    struct HashCompare {
        bool operator()(const Hash256& a, const Hash256& b) const;
    };
    std::set<Hash256, HashCompare> seenEventIds;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP
```

### Invariants:
* `logEvent` returns `false` gracefully if bounds or rate limits are exceeded, ensuring no crashes on consensus paths.
* `seenEventIds` ensures deterministic duplicate suppression across the epoch.

---

## 3. Proof Generation and Validation

At the epoch boundary, the node binds its `AmbientContributionSummary` to its cryptographic identity and energy profile to form a single `AmbientParticipationProof`.

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_PROOFS_HPP
#define AMBIENT_AI_PARTICIPATION_PROOFS_HPP

#include <vector>
#include <string>
#include <array>
#include "ambient_participation_measurement.hpp"

namespace ailee {
namespace participation {

struct AmbientParticipationProofHeader {
    std::string publicKeyHex;         // Binds directly to IdentityPayload::publicKeyHex
    uint64_t epochId;                 // Monotonically increasing L2 epoch counter
    Hash256 summaryHash;              // Hash of the AmbientContributionSummary
    int64_t derivedEnergyScore;       // Sourced from energy::EnergyProfile::calculateEpochEnergy()

    // Deterministic binary serialization
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

struct AmbientParticipationProof {
    AmbientParticipationProofHeader header;
    AmbientContributionSummary summary;
    std::vector<uint8_t> ecdsaSignature; // Strict DER-encoded secp256k1 signature over header.hash()

    // Validation rules:
    // 1. Verifies the ECDSA signature matches publicKeyHex.
    // 2. Asserts summary.hash() == header.summaryHash.
    // 3. Asserts derivedEnergyScore >= 0.
    // 4. Asserts limits via ParticipationLimits.
    bool verify() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_PROOFS_HPP
```

### Invariants:
* A node produces exactly one `AmbientParticipationProof` per epoch.
* `ecdsaSignature` strictly uses `libsecp256k1` primitives.
* Validation failure results in the entire proof being deterministically dropped from the epoch aggregate.

---

## 4. Anchoring Proofs into Bitcoin L2

Proofs are not broadcast randomly. They are aggregated locally, verified, and merkleized by the L2 BlockProducer.

### Integration with `AmbientAIAnchoringEpoch`
* **Aggregation:** During epoch transition, the BlockProducer collects all verified `AmbientParticipationProof` structures from participating mesh nodes.
* **Merkleization:** The `header.hash()` of each verified proof forms the leaves of a Merkle tree. The proofs are deterministically sorted lexicographically by `header.publicKeyHex` before hashing.
* **Commitment:** The resulting Merkle root becomes the `participationRoot` within the `protocol::AmbientAIAnchoringEpoch` struct defined in `bitcoin_anchoring_layer_spec.md`.
* **Data Availability:** The raw `AmbientParticipationProof` data is serialized and pushed to the Data Availability layer alongside the L2 State Diff, ensuring full reconstructability while keeping the Taproot footprint minimal (32 bytes).

---

## 5. Bitcoin "Give-Back" Mechanism (Non-Monetary)

Ambient nodes utilize their participation proofs to generate non-monetary prioritization hints. These hints reward strong contributors with better routing, coordination, and relay precedence for L1 transactions without altering Bitcoin's monetary policy.

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP
#define AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace ailee {
namespace participation {

// Encapsulates a coordination and routing advantage signal
struct AmbientReputationSignal {
    std::string publicKeyHex;
    uint32_t reliabilityTier;    // Deterministically bucketed (e.g., 1 to 5)
    uint64_t cumulativeUptime;   // Sourced from multi-epoch participation records
};

// A hint used by L2 coordination nodes to bias L1 relay and L2 mesh routing
struct AmbientGiveBackHint {
    AmbientReputationSignal signal;

    // Deterministic routing boost applied to ReputationRateLimiter.
    // e.g., higher reputation yields a larger forward rate multiplier.
    int32_t calculateRoutingMultiplier() const;

    // Generates a priority score used to prefer this node as a relay candidate
    // for anchoring Bitcoin L1 transactions.
    uint32_t calculateL1RelayPriority() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP
```

### Invariants:
* `AmbientGiveBackHint` never issues tokens or alters fees.
* Coordination nodes deterministically prefer nodes with higher `calculateL1RelayPriority()` for gossiping Taproot anchor transactions to the Bitcoin Mempool.
* Routing hints directly feed into the mesh layer's `ReputationRateLimiter` and `NeighborEntry::cachedEnergyScore`.

---

## 6. Full Execution Plan

### Phase 1: Contribution Definitions & Measurement Logistics
1. **Implementation:**
   - Define `AmbientContributionEvent`, `AmbientContributionBucket`, and `AmbientContributionSummary`.
   - Implement `AmbientContributionLog` enforcing `ParticipationLimits` (rate limiting, duplicate suppression).
2. **Integration:** Connect log inputs to `AmbientMeshRoutingEngine` and `AmbientNodeMode` sensing logic.

### Phase 2: Proof Generation & Cryptographic Binding
1. **Implementation:**
   - Define `AmbientParticipationProof` and `AmbientParticipationProofHeader`.
   - Implement proof generation using `libsecp256k1` and energy derivation from `EnergyProfile`.
2. **Integration:** Tie proof generation into the epoch boundary transition within the L2 BlockProducer.

### Phase 3: Bitcoin L2 Anchoring & Merkleization
1. **Implementation:**
   - Implement deterministic sorting and Merkle root calculation for the proofs.
   - Inject the resulting hash into `AmbientAIAnchoringEpoch::participationRoot`.
2. **Integration:** Plumb the full raw proofs into the DA payload serialization alongside `L2StateDiff`.

### Phase 4: Non-Monetary Give-Back Mechanics
1. **Implementation:**
   - Define `AmbientGiveBackHint` and `AmbientReputationSignal`.
   - Update `ReputationRateLimiter` to ingest `calculateRoutingMultiplier()`.
2. **Integration:** Update the L1 relay logic to bias transaction propagation based on `calculateL1RelayPriority()`.

### Design-Level Test Scenarios

1. **Test: Adversarial Node Spamming Events**
   - **Scenario:** An adversarial node injects 10,000 sensing events in a single epoch.
   - **Expected Outcome:** `AmbientContributionLog` deterministically truncates the ingestion at `MAX_EVENTS_PER_EPOCH` and `MAX_SENSING_PER_TICK`. The resulting `AmbientContributionSummary` remains strictly bounded, and no crashes occur.

2. **Test: Replay Protection across Epochs**
   - **Scenario:** A node attempts to replay an `AmbientParticipationProof` or a sub-event from Epoch N into Epoch N+1.
   - **Expected Outcome:** `verify()` strictly fails because `header.epochId` mismatch invalidates the signature, and event-level duplication within the epoch is blocked by `seenEventIds`.

3. **Test: Mobile vs Desktop Reproducibility**
   - **Scenario:** A low-power ARM mobile node and an x86_64 desktop node both process the same set of mesh participation events.
   - **Expected Outcome:** Both independently generate the exact same `AmbientContributionSummary` byte array, confirming fixed-point arithmetic, endian-agnostic serialization, and strictly deterministic logical ticking bounds.

4. **Test: L1 Relay Prioritization Validation**
   - **Scenario:** A network of 5 nodes exists; one node has a significantly higher `derivedEnergyScore` across multiple epochs.
   - **Expected Outcome:** The coordination logic deterministically selects the high-reputation node as the primary relay candidate for broadcasting the Taproot Anchor transaction to the L1 mempool, validating the "give-back" mechanism.
