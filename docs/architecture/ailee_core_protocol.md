# AILEE CORE Protocol Layer

The AILEE CORE Protocol layer provides the deterministic bridging logic that unifies the foundational Ambient AI (V10) substrate with the Wave Native Network (WNN - V11) phase coherence logic.

## 1. Architectural Role
AILEE CORE does not execute arbitrary non-deterministic actions. Instead, it serves as the strict integration mechanism connecting:
- **V10 Substrate:** Provides verifiable anchoring, L2 merkleization, logical tick bounding, energy metrics, and participation structures.
- **V11 WNN Layer:** Provides physical phase simulation (Duffing Oscillator), resonance profiling, phase-locked loops (PLL), and transduction routing.

## 2. Integration Vectors

### 2.1 Runtime Integration
`ailee_core_runtime` binds the physical continuous simulation of WNN to the logical tick simulation of the Ambient L2 protocol.
- **WaveState to AmbientRuntimeScheduler:** `WaveState` timestamp representations are deterministically converted from logical ticks, ensuring WNN step operations strictly align with L1 anchored boundaries.

### 2.2 Identity Binding
`ailee_core_identity` ties WNN phase signatures (`PeerId`) and Phase Coherence Anchor clusters back to cryptographic identity.
- **PeerId to IdentityPayload:** WNN phase identities are serialized and bound into the `IdentityPayload` allowing phase trust scaling based on cryptographic signatures.

### 2.3 Participation & Resonance
`ailee_core_participation` governs how physical wave resonance capacities influence standard network energy constraints.
- **CoherenceCluster to ParticipationProof:** Hardware resonance profiles dictate bounded multipliers on base compute capacity ensuring deterministic rewards scaling without token issuance.

### 2.4 Consensus State Machine
`ailee_core_consensus` protects standard consensus phases (like `COLLECT`) from raw analogue interference or Replay Attacks detected at the phase level.
- **PhaseStateMachine to AmbientConsensusStateMachine:** WNN Distributed PLL Replay detection immediately forces the L2 Consensus into a protected `RECOVERY` phase.

### 2.5 Mesh Routing Integration
`ailee_core_mesh` filters network messaging across dual validation pipelines.
- **RoutingEngine to AmbientMesh:** Phase divergence, latency, and standard L2 semantic topology limits (`MAX_MESSAGES_PER_SECOND`, `MAX_HOP_COUNT`) strictly govern transduction pathings.

### 2.6 Merkleization & Anchoring
`ailee_core_anchoring` serializes the state of the physical phase network into verifiable proofs.
- **WaveStateCommitment to AmbientL2Merkleizer:** Live Continuous `WaveState` parameters (like Omega, and Phase Theta) are strictly copied via IEEE-754 binary serialization and accumulated directly into the `AmbientEventRoot` for Taproot commitment.
