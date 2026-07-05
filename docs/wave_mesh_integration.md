# Wave Mesh Integration

WNN does not exist in a vacuum; it orchestrates its continuous wave state over a fluid topology of nodes. This macro-level network awareness is managed by the `MeshOrchestrator`.

## 1. Mesh Node State Representation

The `MeshOrchestrator` aggregates the status of all visible nodes into a discrete state struct that acts as an input for higher-level policy decisions.

```cpp
struct MeshNodeState {
    PeerId peer_id;
    std::optional<wnn::space::AnchorId> anchor_id;
    double mesh_density_local;
    double trust_score;
    double phase_error_deg;
    bool pll_locked;
    bool coherence_stable;
};
```

## 2. Mesh Orchestration Logic

The `MeshOrchestrator` is stepped every tick loop by the `wavefrontd` daemon. Its primary function is to evaluate the real-time health of the network topology.

It calculates the ratio of "healthy" nodes versus total tracked nodes.
A node is considered healthy if:
- Its `trust_score` is above the configured `mesh_min_trust_threshold`.
- Its `phase_error_deg` is below the configured `mesh_max_phase_error_deg`.
- Its `pll_locked` flag is true (if it represents an anchor).
- Its `coherence_stable` flag is true (if it represents an anchor).

If the ratio of healthy nodes drops below `mesh_min_healthy_node_ratio` (e.g., 0.60), the overall `mesh_healthy_` flag goes false.

## 3. Coherence Clusters Integration

The orchestrator integrates directly with the `CoherenceEngine`. It accepts `CoherenceCluster` vectors, which allow the orchestrator to map abstract `PeerId` nodes to highly trusted physical reference points (`AnchorId`). The stability of these clusters provides a baseline for determining if local node phase drift is a localized hardware issue or a network-wide topological shift.

## 4. Synchronization Integration

The output of the `MeshOrchestrator` feeds two critical subsystems:
1. **Safety Guardrails:** Sustained mesh health degradation triggers the `MeshHealthDegraded` incident, potentially activating the Go-Dark protocol.
2. **Routing Engine:** The `RoutingEngine` observes the orchestrator's state to determine the `RoutingMode`. If the mesh density is dropping or the network PLL is unlocked, the engine aggressively shifts to `HIGH_STABILITY` mode to prevent signal fragmentation.
