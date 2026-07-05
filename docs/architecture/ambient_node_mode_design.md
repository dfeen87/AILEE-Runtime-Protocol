# Ambient Node Mode Design for AILEE CORE + Ambient-AI
## Protocol-Grade Architectural Specification

This document defines the deterministic "Ambient Node Mode" that allows low-power devices (phones, laptops) running AILEE CORE to participate in the Ambient-AI mesh and contribute to Bitcoin L2–anchored ambient infrastructure.

This design is strictly deterministic, privacy-aware, low-power, and serves as an implementation-ready blueprint compatible with `bitcoin_anchoring_layer_spec.md` and `ambient_ai_primitives_spec.md`.

---

## 1. Module Structure (AmbientNodeMode Architecture)

The Ambient Node Mode is logically isolated into a new directory: `src/ambient_node/`. It features clear boundaries between sensing, routing, L2 interaction, and participation tracking.

```cpp
#ifndef AMBIENT_AI_NODE_MODE_CONTROLLER_HPP
#define AMBIENT_AI_NODE_MODE_CONTROLLER_HPP

#include <memory>
#include <cstdint>

namespace ailee {
namespace ambient_node {

// Forward declarations
class AmbientSensingEngine;
class AmbientRoutingEngine;
class AmbientL2Bridge;
class AmbientParticipationTracker;
struct AmbientNodeConfig;

// The main controller managing the lifecycle of Ambient Node Mode.
class AmbientNodeModeController {
public:
    explicit AmbientNodeModeController(const AmbientNodeConfig& config);
    ~AmbientNodeModeController();

    // Hooks directly into AILEE CORE startup.
    // Fails fast if invariants (e.g., BuildInfo metadata mismatch) fail.
    bool initialize();

    // Hooks directly into AILEE CORE shutdown.
    void shutdown();

    // Exposes current state for graceful degradation checks by the main node.
    bool isOperatingNominally() const;

private:
    std::unique_ptr<AmbientSensingEngine> sensingEngine;
    std::unique_ptr<AmbientRoutingEngine> routingEngine;
    std::unique_ptr<AmbientL2Bridge> l2Bridge;
    std::unique_ptr<AmbientParticipationTracker> participationTracker;

    AmbientNodeConfig config;
    bool isRunning;
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_NODE_MODE_CONTROLLER_HPP
```

### Invariants:
* `AmbientNodeModeController` owns its subsystems via `std::unique_ptr`, ensuring deterministic initialization and destruction order.
* The controller gracefully handles initialization failure by safely remaining in an `OFF` state without crashing the main AILEE CORE node.

---

## 2. Ambient Signals and Privacy-Preserving Processing

All ambient signals are heavily discretized and processed locally. Raw sensitive data is never captured, stored, or transmitted.

```cpp
#ifndef AMBIENT_AI_SENSING_ENGINE_HPP
#define AMBIENT_AI_SENSING_ENGINE_HPP

#include <cstdint>
#include <vector>

namespace ailee {
namespace ambient_node {

// Deterministic bucketing enums for privacy preservation
enum class PowerState : uint8_t { LOW = 0, MEDIUM, HIGH };
enum class LocationClass : uint8_t { UNKNOWN = 0, INDOOR, URBAN_ZONE, RURAL_ZONE };
enum class ConnectivityBand : uint8_t { POOR = 0, FAIR, GOOD, EXCELLENT };
enum class TimeOfDayBand : uint8_t { MORNING = 0, AFTERNOON, EVENING, NIGHT };

// A strictly sanitized, privacy-preserved snapshot of device state.
struct AmbientSignalBucket {
    PowerState power;
    LocationClass location;
    ConnectivityBand connectivity;
    TimeOfDayBand timeOfDay;

    // Serialization must be stable across all architectures
    std::vector<uint8_t> serialize() const;
};

struct AmbientSignalSnapshot {
    uint64_t logicalTimestamp; // Protocol-defined tick
    AmbientSignalBucket bucket;

    // Hash based on bucket state + timestamp
    std::vector<uint8_t> hash() const;
};

class AmbientSensingEngine {
public:
    // Takes a deterministic snapshot of discretized signals based on sampling cadence
    AmbientSignalSnapshot sample() const;

    // Aggregates snapshots over a defined window (e.g., 10 minutes)
    std::vector<AmbientSignalSnapshot> getAggregatedWindow() const;

    // Prunes snapshots older than the configured retention window
    void pruneOldSnapshots(uint64_t currentLogicalTimestamp);
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_SENSING_ENGINE_HPP
```

### Invariants:
* `AmbientSignalBucket` must only contain coarse-grained, enumerated types. Exact GPS coordinates or battery percentages are strictly forbidden.
* Aggregation and pruning rules rely on deterministic logical timestamps, not `std::chrono::system_clock`.
* `AmbientSensingEngine` never accesses disk directly; all operations act on volatile memory.

---

## 3. Low-Power Micro-Routing Between Nearby Nodes

Routing operates under strict low-power constraints.

```cpp
#ifndef AMBIENT_AI_ROUTING_ENGINE_HPP
#define AMBIENT_AI_ROUTING_ENGINE_HPP

#include <cstdint>
#include <vector>

namespace ailee {
namespace ambient_node {

struct AmbientLowPowerProfile {
    uint32_t maxConcurrentConnections = 5;
    uint32_t maxMessagesPerSecond = 10;
    uint32_t maxBytesPerSecond = 5120; // 5 KB/s
    PowerState minBatteryThreshold = PowerState::MEDIUM;
};

// Represents the state of a micro-routing peer connection
enum class RoutingState : uint8_t { DISCONNECTED, CONNECTING, CONNECTED, BACKOFF };

class AmbientRoutingEngine {
public:
    explicit AmbientRoutingEngine(const AmbientLowPowerProfile& profile);

    // Determines if routing can proceed based on low-power constraints and battery state
    bool canRoute(PowerState currentPower) const;

    // Attempts to route a small ambient coordination message.
    // Returns false if rate limits or battery thresholds are exceeded.
    bool routeMessage(const std::vector<uint8_t>& message);

    // Manages deterministic backoff windows if a peer is unreachable
    void handleFailure(const std::string& peerId);

    // Tries to reconnect based on deterministic retry windows
    void processRetries(uint64_t logicalTimestamp);

private:
    AmbientLowPowerProfile profile;
    // Internal state tracking peer connections, rate limits, and backoff timers
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_ROUTING_ENGINE_HPP
```

### Invariants:
* `AmbientRoutingEngine` enforces caps immediately; excess messages are deterministically dropped, not queued indefinitely.
* Reconnections to peers use a deterministic backoff strategy dependent on logical ticks.
* If `PowerState` falls below `minBatteryThreshold`, all active routing attempts cease, degrading gracefully.

---

## 4. Deterministic Participation Tracking

Participation builds the foundation for Ambient Participation Proofs without tracking sensitive data.

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_TRACKER_HPP
#define AMBIENT_AI_PARTICIPATION_TRACKER_HPP

#include <cstdint>
#include <vector>
#include <array>

namespace ailee {
namespace ambient_node {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientParticipationRecord {
    uint32_t successfulRoutes;
    uint32_t failedRoutes;
    uint32_t validSnapshotsTaken;
    uint64_t uptimeSegments; // Count of valid L2 heartbeat ticks

    // Accrues deterministic participation metrics
    void accrueRoute(bool success);
    void accrueSnapshot();
    void accrueUptime();
};

struct AmbientParticipationEpochSummary {
    uint64_t epochId;
    AmbientParticipationRecord record;

    // Binds directly to ailee::energy::EnergyProfile mapping
    Hash256 computeParticipationHash() const;
    std::vector<uint8_t> serializeForEnergyProfile() const;
};

class AmbientParticipationTracker {
public:
    // Tracks ongoing participation during the current epoch
    void update(const AmbientParticipationRecord& update);

    // Finalizes the epoch summary for ingestion into the anchoring layer
    AmbientParticipationEpochSummary finalizeEpoch(uint64_t epochId) const;

    // Clears the internal state for the new epoch
    void resetForNewEpoch();
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_TRACKER_HPP
```

### Invariants:
* Metrics only track counts and intervals. No user behavior or exact timings are captured.
* `AmbientParticipationEpochSummary` feeds deterministically into the `EnergyProfile` calculation rules defined in `ambient_ai_primitives_spec.md`.

---

## 5. Minimal Changes to AILEE CORE

Ambient Node Mode is opt-in, driven by injected configuration, and does not alter core behavior if disabled.

```cpp
#ifndef AMBIENT_AI_NODE_CONFIG_HPP
#define AMBIENT_AI_NODE_CONFIG_HPP

#include "ambient_routing_engine.hpp" // For AmbientLowPowerProfile

namespace ailee {
namespace ambient_node {

struct AmbientNodeConfig {
    bool ambientNodeModeEnabled = false; // OFF by default
    AmbientLowPowerProfile lowPowerProfile;

    // Safety thresholds
    uint32_t cpuLoadThresholdPercent = 20; // Max CPU impact
};

} // namespace ambient_node

namespace core {

// Hypothetical injection point within AILEE CORE Main Controller
class CoreController {
public:
    // Core Controller accepts the ambient config and optionally instantiates AmbientNodeModeController
    void initialize(const ambient_node::AmbientNodeConfig& ambientConfig) {
        if (ambientConfig.ambientNodeModeEnabled) {
            // instantiate ambient_node::AmbientNodeModeController
            // ...
        }
    }
};

} // namespace core
} // namespace ailee

#endif // AMBIENT_AI_NODE_CONFIG_HPP
```

### Invariants:
* Ambient mode defaults to `OFF`.
* AILEE CORE main components (`BlockProducer`, `Mempool`) are completely unblocked and unaware of Ambient operations unless explicitly fed `AmbientParticipationEpochSummary` data during epoch boundary formation.

---

## 6. Step-by-Step Execution Plan

**Phase 1: Scaffolding and Configuration Wiring**
1. Create `src/ambient_node/` directory.
2. Implement `ambient_node_config.hpp` and wire the config struct into the main `CoreController` or equivalent entry point.
3. Implement `AmbientNodeModeController` stub logic (startup/shutdown hooks).
4. **Validation:** Write C++ unit tests to verify that `ambientNodeModeEnabled = false` results in zero allocation and zero side-effects.

**Phase 2: Privacy-Preserving Sensing Engine**
1. Implement `AmbientSignalBucket` enums and structures.
2. Implement `AmbientSensingEngine` logic using fixed logical sampling windows.
3. **Validation:** Create unit tests verifying data discretization (no raw values) and deterministic hashing of `AmbientSignalSnapshot`s across different simulated environments.

**Phase 3: Low-Power Micro-Routing Engine**
1. Implement `AmbientRoutingEngine` state machine handling rate caps, backoffs, and battery bounds.
2. Integrate with `AmbientLowPowerProfile`.
3. **Validation:** Implement unit tests proving that exceeding `maxMessagesPerSecond` or dropping below `minBatteryThreshold` immediately transitions the state machine to reject further routes deterministically.

**Phase 4: Participation Tracking and Anchoring Integration**
1. Implement `AmbientParticipationTracker` and its `AmbientParticipationRecord` structs.
2. Build the bridging logic (`AmbientL2Bridge` - header omitted for brevity but implied) that takes `AmbientParticipationEpochSummary` and translates it into inputs for `EnergyProfile`.
3. **Validation:** Verify epoch boundaries. Test that accumulating X routes and Y snapshots yields the exact deterministically expected `computeParticipationHash()`.

**Phase 5: System Verification and Resource Profiling**
1. Run local testnets with ambient mode enabled on mock simulated low-power configurations.
2. Verify CPU, bandwidth, and memory allocations remain strictly within defined bounds (e.g., `maxBytesPerSecond`).
3. Ensure no regressions occur in L2 Liveness or consensus processing within standard AILEE CORE execution.
