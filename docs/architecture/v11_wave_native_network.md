# V11 Wave Native Network (WNN) Architecture

The Wave Native Network (WNN) introduces a continuous physical state overlay onto the AILEE Protocol, prioritizing physical resonance and phase-locking over discrete routing payloads.

## Core Subsystems
The V11 protocol logic is located under `src/wnn/` and handles the discrete continuous physical simulations defining the phase state of a Node.

### 1. WNN Core
Contains the `WaveState` representation and `DuffingOscillator` stepper, utilizing IEEE-754 precision math (Kahan summation) to guarantee deterministic phase evolution.

### 2. Propagation
Manages Wave Refraction and Inter-Arrival Time (IAT) Transduction.

### 3. Resonance
Defines node-specific calibration techniques mapping `alpha`, `beta`, `gamma`, and `omega` parameters from native hardware constraints.

### 4. Identity
Evaluates trust purely on phase stability, defining `PeerId`s by unencoded physical signatures and analyzing alignment to Phase Coherence Anchors.

### 5. Routing
Provides an Adaptive Phase Routing Engine calculating Hybrid Phase-Space Distances based on continuous jitter and phase drift instead of geolocation logic.

### 6. State Machine
Utilizes Schmitt-trigger gating and Spiral-Time Phase Salting (derived via deterministic hashing) to enforce replay protection against continuous state drift.

## Integration
WNN relies wholly on `ailee_core` implementations to be bound into the network. It does not possess its own independent start routines or threads, guaranteeing continuous simulations remain anchored to Bitcoin L1 block timings managed by V10.
