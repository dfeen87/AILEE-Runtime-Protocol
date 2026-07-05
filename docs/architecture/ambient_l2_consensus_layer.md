# Ambient Internet L2 Consensus Layer
## Protocol-Grade Architectural Specification

### 1. Define the Ambient L2 Consensus Model

#### Invariants:
* Consensus strictly operates based on deterministic L1 block height and L2 logical timestamps.
* State and view representations strictly use byte-aligned structures with deterministic serialization.
* Epoch boundaries are triggered explicitly by Bitcoin L1 block heights.

```cpp
#ifndef AMBIENT_AI_L2_CONSENSUS_MODEL_HPP
#define AMBIENT_AI_L2_CONSENSUS_MODEL_HPP

#include <vector>
#include <array>
#include <cstdint>

namespace ailee {
namespace protocol {
    struct AmbientAIAnchoringEpoch;
}
namespace identity {
    struct ParticipationProof;
    struct IdentityPayload;
}
namespace energy {
    struct EnergyProfile;
    struct EnergyLedgerEntry;
}
namespace ambient {
    struct AmbientEvent;
}
namespace ambient_mesh {
    struct AmbientMeshParticipationSummary;
}
}

namespace ailee {
namespace consensus {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientL2ConsensusState {
    uint64_t currentEpochId;
    uint64_t currentLogicalTimestamp;

    Hash256 l2StateRoot;
    Hash256 energyLedgerRoot;
    Hash256 participationRoot;
    Hash256 ambientEventRoot;

    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

struct AmbientL2ConsensusView {
    AmbientL2ConsensusState stateSnapshot;

    bool verifyInvariants() const;
    Hash256 getSnapshotHash() const;
};

struct AmbientL2EpochBoundary {
    uint64_t previousEpochId;
    uint64_t newEpochId;
    uint64_t targetBitcoinHeight;

    protocol::AmbientAIAnchoringEpoch finalizePreviousEpoch(const AmbientL2ConsensusState& currentState) const;
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_L2_CONSENSUS_MODEL_HPP
```

### 2. Deterministic Ordering Rules

#### Invariants:
* Ordering strictly prohibits nondeterministic tie-breaking or randomness.
* Lexicographical and energy-based sort implementations must behave identically across all architectures.

```cpp
#ifndef AMBIENT_AI_CONSENSUS_ORDERING_RULES_HPP
#define AMBIENT_AI_CONSENSUS_ORDERING_RULES_HPP

#include <vector>
#include <string>

namespace ailee {
namespace identity {
    struct ParticipationProof;
}
namespace ambient_mesh {
    struct AmbientMeshParticipationSummary;
}
namespace ambient {
    struct AmbientEvent;
}
namespace energy {
    struct EnergyProfile;
}
}

namespace ailee {
namespace consensus {

struct AmbientConsensusOrderingRules {
    static void orderParticipationProofs(std::vector<identity::ParticipationProof>& proofs);

    static void orderEnergyProfiles(std::vector<energy::EnergyProfile>& profiles);

    struct LabeledMeshSummary {
        std::string peerId;
        ambient_mesh::AmbientMeshParticipationSummary summary;
    };
    static void orderMeshSummaries(std::vector<LabeledMeshSummary>& summaries);

    static void orderAmbientEvents(std::vector<ambient::AmbientEvent>& events);
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_CONSENSUS_ORDERING_RULES_HPP
```

### 3. Consensus State Machine

#### Invariants:
* State transitions must progress sequentially unless a recovery phase is strictly triggered.
* State transitions cannot rely on wall-clock time (`std::chrono`), solely on L2 logical timestamps and L1 block height.

```cpp
#ifndef AMBIENT_AI_CONSENSUS_STATE_MACHINE_HPP
#define AMBIENT_AI_CONSENSUS_STATE_MACHINE_HPP

#include <cstdint>

namespace ailee {
namespace consensus {

enum class ConsensusPhase : uint8_t {
    INIT = 0,
    COLLECT = 1,
    ORDER = 2,
    MERKLEIZE = 3,
    ANCHOR = 4,
    FINALIZE = 5,
    RECOVERY = 6
};

struct AmbientConsensusTransition {
    ConsensusPhase fromPhase;
    ConsensusPhase toPhase;
    uint64_t logicalTimestamp;

    bool isValidTransition() const;
};

class AmbientConsensusStateMachine {
public:
    AmbientConsensusStateMachine();

    ConsensusPhase getCurrentPhase() const;

    bool advancePhase(const AmbientConsensusTransition& transition);

    bool initiateRecovery();

private:
    ConsensusPhase currentPhase;

    bool validateCollectPhaseInvariants() const;
    bool validateOrderPhaseInvariants() const;
    bool validateMerkleizePhaseInvariants() const;
    bool validateAnchorPhaseInvariants() const;
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_CONSENSUS_STATE_MACHINE_HPP
```

### 4. Merkleization and Taproot Anchoring

#### Invariants:
* Merkle tree construction must strictly concatenate byte arrays `SHA256(left || right)`.
* Hashing must use `libsecp256k1` primitives explicitly.

```cpp
#ifndef AMBIENT_AI_L2_MERKLEIZATION_HPP
#define AMBIENT_AI_L2_MERKLEIZATION_HPP

#include <vector>
#include <array>
#include <cstdint>

namespace ailee {
namespace protocol {
    struct AmbientAIAnchoringEpoch;
}
namespace anchoring {
    class TaprootAnchor;
}
}

namespace ailee {
namespace consensus {

using Hash256 = std::array<uint8_t, 32>;

class AmbientL2Merkleizer {
public:
    Hash256 computeMerkleRoot(const std::vector<Hash256>& leaves) const;

    protocol::AmbientAIAnchoringEpoch buildEpochCommitment(
        uint64_t epochId,
        uint64_t startHeight,
        uint64_t endHeight,
        const Hash256& l2StateRoot,
        const Hash256& energyLedgerRoot,
        const Hash256& participationRoot,
        const Hash256& ambientEventRoot
    ) const;
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_L2_MERKLEIZATION_HPP
```

### 5. Bitcoin-Compatible “Consensus Benefits”

#### Invariants:
* Benefits provide local coordination advantages only.
* No monetary rewards, tokens, or network issuance are supported.

```cpp
#ifndef AMBIENT_AI_CONSENSUS_BENEFITS_HPP
#define AMBIENT_AI_CONSENSUS_BENEFITS_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace ailee {
namespace consensus {

struct AmbientConsensusReputationSignal {
    std::string publicKeyHex;
    int64_t energyScore;
    bool isParticipationVerified;

    std::vector<uint8_t> serialize() const;
};

struct AmbientConsensusBenefit {
    std::string publicKeyHex;

    uint32_t routingPriorityMultiplier;

    uint32_t relayPriorityMultiplier;

    void calculateFromReputation(const AmbientConsensusReputationSignal& signal);
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_CONSENSUS_BENEFITS_HPP
```

### 6. Full Execution Plan

* **Implementation Phases:**
  * **Phase 1: Structs & Data Types:** Implement `AmbientL2ConsensusState`, `AmbientL2ConsensusView`, and `AmbientL2EpochBoundary` ensuring deterministic serialization using strictly bounded types.
  * **Phase 2: Ordering Rules:** Implement `AmbientConsensusOrderingRules` matching exact lexicographical and fixed-point math logic to guarantee zero variations in peer rankings.
  * **Phase 3: State Machine Logic:** Hook up `AmbientConsensusStateMachine` using exclusively logical timestamps. Guarantee no system clock dependencies exist in transition checks.
  * **Phase 4: Merkle Construction:** Implement `AmbientL2Merkleizer` ensuring libsecp256k1 wrappers are utilized for all SHA256 iterations matching Bitcoin Taproot requirements.
  * **Phase 5: Benefit Assignment:** Implement `AmbientConsensusBenefit` calculating coordination hints.

* **Integration Steps:**
  * Interface `AmbientL2EpochBoundary` with L1 block notifications received from `IBitcoinNodeView`.
  * Pipe sorted `AmbientMeshParticipationSummary` and `EnergyProfile` elements directly into the state machine's `ORDER` phase.
  * Extract output commitments into `TaprootAnchor` integration for Bitcoin L1 injection.

* **Test Scenarios:**
  * **Consensus Under Load:** Simulate 1M ordered events and benchmark memory/CPU footprints on desktop versus constrained environments.
  * **Partial Connectivity:** Simulate node dropouts during `COLLECT` phase; test deterministic fallback to `RECOVERY`.
  * **Adversarial Nodes:** Inject duplicate identities and conflicting logical timestamps; assert deterministic pruning in `ORDER` phase.
  * **Mobile vs Desktop Reproducibility:** Run full L2 execution across ARM64 (Mobile) and x86_64 (Desktop); assert `Hash256` equivalence byte-for-byte at `MERKLEIZE` phase.

* **Resource Usage Validation:**
  * Ensure maximum execution time per `advancePhase` transition remains below 10ms threshold.
  * Ensure heap memory allocations are strictly bound for large Merkle tree calculations.
