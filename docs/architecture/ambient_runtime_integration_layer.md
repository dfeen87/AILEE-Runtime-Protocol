# Ambient Internet Runtime Integration Layer
## Protocol-Grade Architectural Specification

This document defines the deterministic execution environment for the Ambient AI L2 network. It unifies consensus, mesh networking, node mode, primitives, and anchoring into a single reproducible runtime.

---

### 1. Ambient Internet Runtime Model

The runtime explicitly governs scheduling and the event loop strictly based on determinism, prohibiting all random number generation, wall-clock timing, and unpredictable thread-scheduling.

```cpp
#ifndef AMBIENT_AI_RUNTIME_MODEL_HPP
#define AMBIENT_AI_RUNTIME_MODEL_HPP

#include <memory>
#include <cstdint>
#include <vector>

// Forward declarations
namespace ailee {
namespace runtime {
    class AmbientRuntimeScheduler;
    class AmbientRuntimeEventLoop;
    struct AmbientRuntimeConfig;
    class AmbientRuntimeStateMachine;
    class AmbientSubsystemContext;
}
}

namespace ailee {
namespace runtime {

class AmbientRuntimeEventLoop {
public:
    // Polls deterministic events based on L2 logical ticks
    // Strictly constrained to avoid tight-loops and excessive CPU usage
    void tick(uint64_t logicalTimestamp);

    // Enqueues deterministic tasks for the runtime scheduler
    bool enqueueTask(const std::vector<uint8_t>& serializedTask);
};

class AmbientRuntimeScheduler {
public:
    // Schedules execution based purely on deterministic ticks and logical time
    void executeScheduledTasks(uint64_t logicalTimestamp);

    // Rejects tasks that violate low-power bounds or L2 consensus timing
    bool validateTaskConstraints(const std::vector<uint8_t>& serializedTask) const;
};

class AmbientRuntime {
public:
    explicit AmbientRuntime(const AmbientRuntimeConfig& config);
    ~AmbientRuntime();

    // Initiates the deterministic runtime loop
    bool initialize();

    // Steps the entire runtime deterministically based on L1 block heights and L2 ticks
    void step(uint64_t logicalTimestamp, uint64_t currentBitcoinHeight);

    // Graceful teardown
    void shutdown();

private:
    std::unique_ptr<AmbientRuntimeScheduler> scheduler;
    std::unique_ptr<AmbientRuntimeEventLoop> eventLoop;
    std::unique_ptr<AmbientRuntimeStateMachine> stateMachine;
    std::unique_ptr<AmbientSubsystemContext> subsystemContext;
    bool isRunning;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_MODEL_HPP
```

### 2. Deterministic Execution Phases

The runtime explicitly moves through deterministic phases to sequence L2 execution cleanly, guaranteeing state derivations cannot interleave non-deterministically.

```cpp
#ifndef AMBIENT_AI_RUNTIME_PHASES_HPP
#define AMBIENT_AI_RUNTIME_PHASES_HPP

#include <cstdint>

namespace ailee {
namespace runtime {

enum class AmbientRuntimePhase : uint8_t {
    INIT = 0,
    LOAD_IDENTITY = 1,
    LOAD_ENERGY = 2,
    LOAD_NODE_MODE = 3,
    LOAD_MESH = 4,
    LOAD_L2_CONSENSUS = 5,
    EXECUTE_EPOCH = 6,
    MERKLEIZE = 7,
    ANCHOR = 8,
    FINALIZE = 9,
    RECOVERY = 10
};

struct AmbientRuntimePhaseTransition {
    AmbientRuntimePhase fromPhase;
    AmbientRuntimePhase toPhase;
    uint64_t logicalTimestamp;
    uint64_t targetBitcoinHeight; // Required for execution boundary changes

    // Validates deterministic transitions without wall-clock reliance
    bool isValid() const;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_PHASES_HPP
```

### 3. Subsystem Integration Rules

The runtime encapsulates the exact integration paths where L2 sub-modules interact, explicitly tracking boundaries established in the five canonical spec documents.

```cpp
#ifndef AMBIENT_AI_SUBSYSTEM_INTEGRATION_HPP
#define AMBIENT_AI_SUBSYSTEM_INTEGRATION_HPP

#include <memory>
#include <vector>

// Explicit integration points across the 5 canonical substrates
namespace ailee {
namespace ambient_node { class AmbientNodeModeController; }
namespace ambient_mesh { class IMeshP2PBridge; }
namespace identity { struct ParticipationProof; }
namespace energy { struct EnergyProfile; }
namespace consensus { class AmbientConsensusStateMachine; }
}

namespace ailee {
namespace runtime {

// Represents a deterministic binding rule between two disjoint systems
struct AmbientSubsystemBinding {
    bool isNodeModeEnabled;
    bool isMeshEnabled;
    bool isConsensusActive;

    // Checks bounds and constraints, e.g., low-power compatibility
    bool validateBindingInvariants() const;
};

class AmbientSubsystemContext {
public:
    // Node mode -> mesh logic binding
    void bridgeNodeModeToMesh(
        ambient_node::AmbientNodeModeController& nodeMode,
        ambient_mesh::IMeshP2PBridge& meshBridge
    );

    // Mesh -> participation proof accumulation
    void aggregateMeshParticipation(
        const ambient_mesh::IMeshP2PBridge& meshBridge,
        std::vector<identity::ParticipationProof>& proofs
    );

    // Participation -> energy ledger calculation
    void computeEnergyFromParticipation(
        const std::vector<identity::ParticipationProof>& proofs,
        std::vector<energy::EnergyProfile>& profiles
    );

    // Energy -> L2 consensus module injection
    void feedEnergyToConsensus(
        const std::vector<energy::EnergyProfile>& profiles,
        consensus::AmbientConsensusStateMachine& consensusMachine
    );

    // Final L2 consensus outputs bind to anchoring via the anchoring pipeline
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_SUBSYSTEM_INTEGRATION_HPP
```

### 4. Runtime State Machine

The runtime employs a state machine isolating subsystem faults and managing phase shifts deterministically on protocol ticks.

```cpp
#ifndef AMBIENT_AI_RUNTIME_STATE_MACHINE_HPP
#define AMBIENT_AI_RUNTIME_STATE_MACHINE_HPP

#include "ambient_runtime_phases.hpp"
#include <vector>
#include <cstdint>

namespace ailee {
namespace runtime {

struct AmbientRuntimeState {
    AmbientRuntimePhase currentPhase;
    uint64_t currentLogicalTimestamp;
    uint64_t lastProcessedBitcoinHeight;

    bool isHealthy;
    uint32_t consecutiveFailures;
};

class AmbientRuntimeStateMachine {
public:
    AmbientRuntimeStateMachine();

    AmbientRuntimeState getCurrentState() const;

    // Deterministically attempts to step the phase based on current context
    bool applyTransition(const AmbientRuntimePhaseTransition& transition);

    // Handles subsystem faults gracefully without crashing AILEE CORE
    void triggerRecovery(uint64_t logicalTimestamp);

private:
    AmbientRuntimeState state;

    bool checkPhaseInvariants(AmbientRuntimePhase phase) const;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_STATE_MACHINE_HPP
```

### 5. Runtime Anchoring Pipeline

Collects all strictly derived proofs and energy ledgers at the epoch termination boundary, producing the final `AmbientAIAnchoringEpoch` for Taproot injection.

```cpp
#ifndef AMBIENT_AI_RUNTIME_ANCHORING_PIPELINE_HPP
#define AMBIENT_AI_RUNTIME_ANCHORING_PIPELINE_HPP

#include <vector>
#include <cstdint>

namespace ailee {
namespace protocol { struct AmbientAIAnchoringEpoch; }
namespace anchoring { class TaprootAnchor; }
namespace ambient { class AmbientEventAggregator; }
namespace identity { struct ParticipationProof; }
namespace energy { struct EnergyProfile; }
}

namespace ailee {
namespace runtime {

class AmbientRuntimeAnchoringPipeline {
public:
    // Collects outputs from the current executed epoch
    void collectEpochOutputs(
        const std::vector<identity::ParticipationProof>& proofs,
        const std::vector<energy::EnergyProfile>& profiles,
        const ambient::AmbientEventAggregator& eventAggregator
    );

    // Performs canonical merkleization of collected outputs strictly using libsecp256k1
    void merkleizeOutputs();

    // Generates the final L2 epoch structure
    protocol::AmbientAIAnchoringEpoch produceFinalEpochRoot(
        uint64_t epochId,
        uint64_t startHeight,
        uint64_t endHeight
    ) const;

    // Commits the epoch root into the Bitcoin Taproot Anchor abstraction
    void commitToTaproot(
        const protocol::AmbientAIAnchoringEpoch& epoch,
        anchoring::TaprootAnchor& anchor
    ) const;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_ANCHORING_PIPELINE_HPP
```

### 6. Global Coordination Fabric

Governs network prioritization explicitly through L2 deterministic state rather than proprietary routing formulas, preserving equal conditions for identical hardware constraints.

```cpp
#ifndef AMBIENT_AI_GLOBAL_COORDINATION_HPP
#define AMBIENT_AI_GLOBAL_COORDINATION_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace ailee {
namespace runtime {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientCoordinationHint {
    std::string peerId;
    int64_t energyScore; // Derived directly from scaled EnergyProfile logic
    uint32_t routingPriorityMultiplier;
    uint32_t relayPriorityMultiplier;

    // Binary serialization strictly bounded and deterministic
    std::vector<uint8_t> serialize() const;
};

struct AmbientCoordinationState {
    uint64_t currentLogicalTimestamp;
    std::vector<AmbientCoordinationHint> networkHints;

    // Generates a deterministic hash representing the collective routing landscape
    Hash256 stateHash() const;
};

class AmbientCoordinationFabric {
public:
    // Derives hints purely from the consensus energy profile array
    std::vector<AmbientCoordinationHint> generateHintsFromEnergy(
        const std::vector<AmbientCoordinationHint>& previousHints
    ) const;

    // Updates the global mesh reputation signals, impacting rate limits globally
    AmbientCoordinationState updateCoordinationState(
        const std::vector<AmbientCoordinationHint>& newHints,
        uint64_t logicalTimestamp
    );
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_GLOBAL_COORDINATION_HPP
```

### 7. Full Execution Plan

#### Implementation Phases
1. **Phase 1: Runtime Foundation:** Implement `AmbientRuntime`, `AmbientRuntimeScheduler`, and `AmbientRuntimeEventLoop` using exclusively logical ticks (no `std::chrono`).
2. **Phase 2: State Machine Mapping:** Develop `AmbientRuntimeStateMachine` covering exact transition invariants from `INIT` through `FINALIZE`.
3. **Phase 3: Integration Matrix:** Construct `AmbientSubsystemContext` explicitly to map `AmbientNodeModeController` state into `EnergyProfile` and `ParticipationProof` structures.
4. **Phase 4: Anchoring Execution:** Deploy `AmbientRuntimeAnchoringPipeline` encapsulating L1 Merkle derivations mapping to `AmbientAIAnchoringEpoch`.
5. **Phase 5: Coordination Rollout:** Wire `AmbientCoordinationFabric` outputs back into runtime scheduling logic for rate prioritization.

#### Integration Steps
- Instantiate `AmbientRuntime` in the AILEE Core boot flow.
- Link ZMQ block listeners directly to `AmbientRuntime::step` triggering L1 height bounds.
- Integrate `IMeshP2PBridge` logic explicitly passing bounded binary serialization into `AmbientSubsystemContext`.

#### Test Scenarios
- **Runtime Under Load:** Mock 50,000 sub-events/sec. Verify `AmbientRuntimeScheduler` deterministically sheds load beyond low-power profile constraints without exception or allocation faults.
- **Partial Subsystem Failure:** Sever local `IMeshP2PBridge` references during the `LOAD_MESH` phase. Assert `AmbientRuntimeStateMachine` increments failure counters and executes `RECOVERY` deterministically.
- **Adversarial Nodes:** Inject duplicate proofs across varied logical timestamps. Assert the anchoring pipeline rejects collisions via fixed-point bounds limits.
- **Mobile vs Desktop Reproducibility:** Run epoch integration pipelines across arm64 and x86_64 hosts; verify all intermediate and root hashes match exactly at byte level.

#### Resource-Usage Validation
- Heap allocations strictly controlled under a capped contiguous arena.
- Event loop polling bounded to fixed L2 ticks avoiding CPU spinning, preserving device autonomy and privacy guarantees.

#### Example Flows
- **Startup:** Process executes `initialize()` -> `stateMachine` sets to `INIT`.
- **Pre-epoch Load:** L1 height match triggers `LOAD_IDENTITY` -> `LOAD_ENERGY` -> `LOAD_NODE_MODE` -> `LOAD_MESH`.
- **Execution:** Enters `EXECUTE_EPOCH` -> Processes logical ticks deterministically aggregating mesh and local proofs.
- **Closure:** Reaches `targetBitcoinHeight` -> Transitions to `MERKLEIZE` -> `ANCHOR` pushes root into `TaprootAnchor` -> `FINALIZE` updates coordination fabric hints.
