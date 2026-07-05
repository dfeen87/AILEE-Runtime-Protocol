# Wave Native Network Core Architecture

The Wave Native Network (WNN) is a deterministic phase-coherence overlay designed for edge synchronization and handoff-stable routing. It utilizes continuous wave states rather than discrete payload routing.

## 1. Deterministic Structures

### WaveState
The core deterministic structure defining the local phase-memory state of a node.
It encapsulates the state of the local physics simulation.

```cpp
struct WaveState {
    double A;       // Amplitude (x)
    double theta;   // Phase accumulator
    double omega;   // Forcing frequency
    double x_dot;   // Velocity
    double ts;      // Continuous continuous timestamp
};
```
This is often expressed mathematically as:
`Ψ(t) = ⟨A(t), θ(t), ω(t), ẋ(t), ts⟩`

## 2. State Machines and Heartbeat

### Node Heartbeat
Each node maintains a local oscillator heartbeat driven by a Duffing-type non-linear equation, acting as a local physics simulation. The equation governs the evolution of the `WaveState`.

Duffing Equation:
`x'' + delta * x' + alpha * x + beta * x^3 = gamma * cos(omega_active * t)`

Where `omega_active` is the forcing frequency, dynamically perturbated by a replay-sensitive phase salt.

The state is deterministically integrated over a microsecond-precision heartbeat loop using a 4th-Order Runge-Kutta (RK4) integrator coupled with Kahan Summation for accumulation variables to ensure rigorous IEEE-754 deterministic integration across billions of cycles. All state variables are modeled strictly with `double` precision (or `long double` in specific engine instances).

## 3. Protocol-Level Orchestration

The centralized policy brain of the system is the **wavefrontd** daemon. It orchestrates the protocol at runtime:
1. **Time-Step Loop:** It executes a strict microsecond-precision tick loop, simulating the local `WaveState` using the `DuffingOscillator`.
2. **Subsystem Integration:** It gathers continuous state streams from:
   - **PhaseCoherence Anchors:** Through the `CoherenceEngine`, evaluating the phase error and cluster stability of known anchor nodes.
   - **Distributed PLL:** Tracking global mesh lock-in.
   - **Mesh Orchestrator:** Determining global network health and density.
3. **Execution Pipeline:** It feeds aggregated results to:
   - **Routing Engine:** Executing the Adaptive Routing logic (Vector A vs. Vector B, mode selection).
   - **Safety Guardrails:** Monitoring threshold breaches (e.g., trust scores, PLL locks, coherence limits).

## 4. Subsystem Interfaces and Consensus

While traditional consensus algorithms rely on discrete cryptographic votes, WNN integrates phase-level consensus:
- **Phase Anchors:** Trusted reference nodes broadcast deterministic signal structures.
- **Distributed Awareness:** The Distributed PLL Controller synchronizes local states with the aggregate phase error of network anchors.
- **Strict Bounded Execution:** All execution paths are deterministic, zero-allocation C++ paths inside the main tick loop, guaranteeing exact phase state calculations across disparate hardware.
