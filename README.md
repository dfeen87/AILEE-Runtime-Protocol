# AILEE Protocol Core for Bitcoin: **Ambient AI + Wave Native Network. Bitcoin Layer-2 Orchestration Framework**

*Building Intelligent, Verifiable, and Sustainable Bitcoin Infrastructure*

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-blue.svg)](https://cmake.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-Python-green.svg)](https://fastapi.tiangolo.com/)
[![Bitcoin](https://img.shields.io/badge/Bitcoin-Layer--2-orange.svg)](https://bitcoin.org/)
[![CI](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/actions/workflows/ci.yml/badge.svg)](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/actions/workflows/ci.yml)


**[Documentation](docs/)** | **[Quick Start](#-quick-start)** | **[API Reference](API_QUICKSTART.md)** | **[Architecture](docs/ARCHITECTURE_CONCEPTUAL.md)** | **[Contributing](docs/CONTRIBUTING.md)**

---

## 📖 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [Testing & Validation](#-testing--validation)
- [Quick Start](#-quick-start)
- [Architecture](#-architecture)
- [Security Model](#-security-model)
- [Technology Stack](#-technology-stack)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

**AILEE-Core** is a Bitcoin Layer-2 orchestration and verification framework. It leverages ambient AI for intelligent task scheduling and a recovery-first design to extend Bitcoin's capabilities without modifying its consensus rules.

---

## ⚠️ Protocol Status & Safety Notice

AILEE Core has evolved into a **deterministic, reproducible Bitcoin L2 protocol** with verified execution paths, canonical state roots, and Wave Native Network integration.  
Version 12 introduces:

- deterministic execution  
- reproducible state transitions  
- frozen genesis  
- canonical build + receipt hashes  
- verified epoch roots (0–4)  
- full re‑execution path  
- automated verification script  

This establishes AILEE Core as a **verifiable protocol**, not a prototype.

All core components in this repository are **real implementations** — no stubs, mocks, or placeholder cryptographic primitives are used. The deterministic runtime, reproducibility package, and Wave Native Network integration are fully functional.

However, before mainnet fund custody, the following production‑grade hardening steps are still required:

- ZK proof system must be upgraded to audited circuits (Halo2, PLONK, Groth16)  
- Signature verification must undergo formal review  
- Networking layers must be security‑audited  
- Third‑party audits are required for any production deployment  

AILEE Core V12 is a **protocol‑grade deterministic release**, but **not yet a mainnet‑ready financial system**.

> “AILEE-CORE is not a prototype anymore — but we’re not a custodial mainnet system.”

### Why AILEE?

1.  **Reorg Awareness**: Built-in reorg detection logs L1 reorganizations to protect L2 state.
2.  **Orchestration Framework**: Ambient AI optimizes task distribution across a decentralized mesh.
3.  **Verifiable (in progress)**: Zero-Knowledge (ZK) proof scaffolding and deterministic state commitments allow for trust-minimized verification once real circuits are integrated.
4.  **Bitcoin-Native**: Respects Bitcoin as the ultimate settlement layer.

---

## Version 12 — Deterministic Release & Wave Native Network Integration

V12 marks a pivotal evolution in the AILEE Protocol Core, introducing strict determinism across the entire system. By decoupling the protocol from local machine dependencies, wall-clock timing, and non-deterministic execution environments, V12 ensures that every state transition is 100% reproducible. This release seamlessly integrates the **Wave Native Network (WNN)**, a foundational architecture for phase-coherent distributed coordination.

### Wave Native Network (WNN) Integration

The **Wave Native Network (WNN)** is a paradigm shift in how decentralized nodes coordinate. Instead of relying on fragile, localized wall-clock timestamps (`std::chrono`) or non-deterministic peer-to-peer gossiping, WNN anchors node communication to a universal, phase-coherent clock signal.

**What it is and why it matters:**
WNN introduces a globally synchronized, deterministic oscillator network. It provides a foundational heartbeat for the AILEE L2, allowing distributed nodes to achieve **timing stability** and **identity coherence** without centralized coordination. WNN ensures that all nodes operate within mathematically precise, collision-resistant phase boundaries.

**Integration into AILEE Core Bitcoin L2:**
Within AILEE Core, WNN guarantees that consensus loops, block production, and federated learning updates occur at precisely verifiable intervals. This eliminates race conditions and non-deterministic timing behaviors, ensuring that distributed coordination is as predictable and stable as the cryptography that secures it. WNN paves the way for a truly ambient, self-orchestrating Bitcoin L2 that functions with machine-like precision.

### V12 Reproducibility Package

To guarantee verifiable execution, V12 introduces a comprehensive Reproducibility Package. This suite of canonical artifacts allows any engineer or automated system to rebuild the binary, run the protocol, and replay state transitions with bit-for-bit accuracy.

*   **Frozen Genesis (`v12_frozen_genesis.json`):** Defines the immutable, deterministic starting state for the protocol, anchoring all subsequent epoch roots.
*   **Build Hash (`v12_build_hash.txt`):** Cryptographically fingerprints the deterministic V12 build environment and compilation manifest.
*   **Receipt Hash (`v12_receipt_hash.txt`):** Fingerprints the deterministic execution outputs and finalized epoch roots.
*   **Re-Execution Path (`v12_reexecution_manifest.json`):** Provides step-by-step, deterministic replay instructions to re-execute the protocol from genesis through epoch 4.
*   **Epoch State Roots:** The verified Merkle roots of the AILEE L2 state after each epoch of execution.
*   **Verification Script (`verify_v12.sh`):** An automated utility to cryptographically validate the integrity of the reproducibility artifacts.

### Canonical Artifacts & Epoch Roots

*   **Build Hash:** `e9576ab0527e7e3fb27230936be817f3cf52fd73f4955a91eae5c282c04bd75a`
*   **Receipt Hash:** `b02a21d807ce351b641bcacadc2f32243e0135996cd8fd34d96a9f71fe8dfdfd`
*   **Re-Execution Hash:** `e62961a5ab1d18eabc01708703eb64b74d6ab0aff569cf7cbafab40e7655b8ca`

**Epoch State Roots:**
*   **Genesis (Epoch 0):** `425e70c240a19594bac19c354a12fa7edbe6e6c6d3c5790e32b5af52de6ff8ee`
*   **Epoch 1:** `9de36bff8690db9a9eb10ee4e1c1277354123b5619f3c74ac6252d67e187011d`
*   **Epoch 2:** `dafeb78382ef48f814a39ea91fad52232ffbfc1fa5976d64a9edafac0f970745`
*   **Epoch 3:** `9e9696973d695c691a4a8c3355ee3246ee92938df6368076d4a6bc8cebc31f47`
*   **Epoch 4:** `f4ffb4ab1eb4e31906c42f16367af8b10664f91876d1c932e1c44f96c0821c0f`

### V12 Verification Script (`verify_v12.sh`)

The `verify_v12.sh` verification script is an automated utility designed to validate the reproducibility package. It programmatically checks the integrity of the V12 build, receipt, and re-execution artifacts by calculating their SHA-256 hashes and comparing them against the canonical hashes listed above. By running this script, auditors and node operators can instantly verify that their local artifacts match the exact, unmodified deterministic state of the V12 release before running the execution replay.

### Why This Matters

V12 and WNN together forge an unbreakable chain of deterministic execution. In decentralized networks, non-determinism is the enemy of consensus. By stripping away reliance on local system clocks, non-deterministic scheduling, and floating-point ambiguity, V12 guarantees that the AILEE L2 operates identically across any hardware. Coupled with WNN's phase-coherent coordination, this ensures reproducible state transitions, eliminating state divergence and providing an incredibly stable, auditable infrastructure for Bitcoin L2 and the broader decentralized internet.

---

## ✨ Key Features

### 🛡️ Reorg Detection
- **RocksDB-Backed Persistence**: Tracks Bitcoin block history and reorg events with high performance.
- **Deep Reorg Detection**: Detects deep blockchain reorganizations (>6 blocks) and logs warnings.
- **State Safety**: Logs reorg events to allow operators to verify L2 state against the new L1 chain.
- **Integration**: Wired to the Bitcoin ZMQ listener for real-time block tracking.

### 🧠 Ambient AI Orchestration
- **Intelligent Task Scheduling**: Weighted, round-robin, least-loaded, and latency-aware strategies.
- **Reputation-Based Assignment**: Node scoring with capacity, cost, and energy considerations.
- **Byzantine Detection**: Outlier-based heuristics for anomaly detection.
- **Deterministic Verification**: Hash-based commitments with integrity checks.

### ⛓️ Bitcoin Layer-2 Infrastructure
- **SPV Verification**: Trust-minimized Bitcoin event verification.
- **Deterministic Anchoring**: OP_RETURN and Taproot commitment construction.
- **Federated Bridge**: Multisig quorum-based peg-in/peg-out lifecycle.
- **Offline Verifier**: `ailee_l2_verify` tool for third-party audits.

### 🌐 Web & API Integration
- **REST API**: FastAPI-based production server with OpenAPI docs.
- **Web Dashboard**: Real-time monitoring interface.
- **Health Checks**: Built-in monitoring and observability.

---

## 🧪 Testing & Validation

**Confidence requires verification.** While AILEE-Core is built with powerful and secure primitives, the responsibility of operating financial infrastructure demands rigorous and continuous testing.

We stress the importance of the following validation steps before any mainnet deployment:

1.  **Unit & Integration Testing**: Run the full test suite (`ctest`) to verify all components, including the new Reorg Detector and Adapter Registry.
2.  **Adversarial Simulation**: Use the `PerformanceSimulator` to stress-test the network under high load and simulated attacks.
3.  **Reorg Simulation**: Test the node's behavior against a simulated Bitcoin regtest network where you intentionally invalidate blocks to trigger the Reorg Detector.
4.  **Audit**: While the architecture is sound, third-party cryptographic audits are recommended for the federation multisig setup.

> **"Trust, but verify."** - The core philosophy of Bitcoin and AILEE.

---

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev \
    libzmq3-dev libcppzmq-dev libjsoncpp-dev libyaml-cpp-dev librocksdb-dev

# macOS
brew install cmake openssl curl zeromq cppzmq jsoncpp yaml-cpp rocksdb
```

### Option 1: Build C++ Core Node

```bash
# Clone the repository
git clone https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin.git
cd AILEE-Protocol-Core-For-Bitcoin

# Build the project
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)

# Run the main node
./ailee_node

# Check logs to verify Reorg Detector initialization
tail -f logs/ailee.log
```

### Option 2: Run Tests

```bash
cd build
ctest --verbose
```

---

## 🏗️ Architecture

<div align="center">

```
┌─────────────────────────────────────────────────────────────────┐
│                      AILEE LAYER-2 NODE                         │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐  │
│  │ Orchestration│  │  Reorg       │  │  Federated         │  │
│  │ & Scheduling │  │  Detector    │  │  Learning          │  │
│  └──────────────┘  └──────────────┘  └────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │            Ambient AI Telemetry & Byzantine Detection    │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              L2 State & Ledger Management                │  │
│  │        (Deterministic Snapshots + Verification)          │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │           Bitcoin Anchoring & Bridge Logic               │  │
│  │  • SPV Peg-In  • Federated Peg-Out  • Commitments       │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │        Multi-Chain Adapters (Bitcoin, ETH, SOL...)      │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              ↕
          ┌───────────────────────────────────────┐
          │     BITCOIN MAINNET (L1)              │
          │  • Anchor Commitments                 │
          │  • Peg-In Verification (SPV)          │
          │  • Peg-Out Settlement (Multisig)      │
          └───────────────────────────────────────┘
```

</div>

### Temporal Architecture Components

- **V28 Governor:** Provides deterministic parameter adjustment and protocol governance, computing absolute bounds from fixed baselines to prevent drift over multi-epoch cycles.
- **V29 Auditor:** Ensures multi-epoch temporal coherence through strict, reproducible metric validations. It mathematically scores state transitions over rolling windows to enforce stability.
- **V30 Energy Resilience Layer (ERL):** Manages Byzantine node resilience and resource allocation deterministically, penalizing unstable actors while ensuring protocol progression under adverse conditions.
- **V31 Intelligence‑Assisted Orchestration (IAO):** Drives the high-level scheduling and execution of epochs, seamlessly integrating state transitions with deterministic ZK proofs and anchored commitments.

### Orchestration Pipeline Overview

The complete temporal architecture of AILEE‑Core functions as a cohesive pipeline (Governor → Auditor → ERL → IAO):
1. **Governor:** Sets deterministic thresholds and bounds for the protocol.
2. **Auditor:** Scrutinizes ongoing epochs against these bounds for temporal coherence and stability.
3. **ERL:** Evaluates node participation and enforces energy-efficient, resilient execution constraints based on Auditor feedback.
4. **IAO:** Orchestrates the final epoch execution, ensuring the transition yields a canonical state root ready for anchoring.

### Design Principles

- **Determinism:** Bit-for-bit reproducible execution across all architectures, stripping away local timing and scheduler ambiguity.
- **Temporal Coherence:** Anchoring network actions to globally synchronized, phase-coherent boundaries rather than local wall-clock timers.
- **Auditable Trust Assumptions:** Explicit, verifiable dependencies mapped to deterministic zero-knowledge proofs and cryptographic commitments.
- **Forward‑Looking Orchestration:** Autonomous, deterministic parameter tuning that adapts to changing network states without losing historical lineage.

---

## 🔐 Security Model

### ✅ Explicit Federated Model

**Peg-Ins (Bitcoin → L2)**
- SPV proofs with Bitcoin headers + Merkle paths
- Trust-minimized verification of L1 events

**Peg-Outs (L2 → Bitcoin)**
- Federated multisig quorum (sidechain-style custody)
- Explicit signer thresholds and fee logic

**Reorg Safety**
- **Automated Detection**: The `ReorgDetector` monitors L1 block headers.
- **Safety Halt**: Block production pauses if a deep reorg is detected.
- **State Recovery**: L2 state can be rolled back to the last finalized anchor.

---

## ⚠️ What This Is NOT

AILEE-Core is **NOT**:
- ❌ A replacement for Bitcoin
- ❌ A consensus-changing protocol
- ❌ A trustless rollup (it is a federated sidechain)

---

## 🛠️ Technology Stack

### Core Infrastructure (C++)
- **Language**: C++17
- **Build System**: CMake 3.10+
- **Persistence**: RocksDB (State & Reorg History)
- **Bitcoin Integration**: JSON-RPC client + ZMQ listener
- **Cryptography**: OpenSSL (SHA256, signature verification)
- **Networking**: libcurl, ZeroMQ, cppzmq
- **JSON Parsing**: nlohmann/json, JsonCpp
- **Configuration**: yaml-cpp, TOML++

### REST API (Python)
- **Framework**: FastAPI (async, production-ready)
- **Server**: Uvicorn (ASGI server)

---

## 🌍 Community & Support

### Get Help
- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Questions and community support
- **Documentation**: Comprehensive guides in `docs/`

### Citation

If you use or build upon this work in academic research, please cite:

```bibtex
@software{ailee_core,
  author = {Feeney, Don Michael},
  title = {AILEE-Core: Bitcoin-Anchored Layer-2 Orchestration Framework},
  year = {2025},
  publisher = {GitHub},
  url = {https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin},
  license = {MIT}
}
```
---

## Acknowledgments

This project was developed with a combination of original ideas, hands‑on coding, and support from advanced AI systems. I would like to acknowledge **Microsoft Copilot** (Trustworthy Assistant, strengthened focus and motivation), **Google Jules** (Code Scaffolding and Code Synthesis Assistance), **Anthropic Claude** (Markdown File formatting assistance), and **OpenAI ChatGPT** (Input on the Original AILEE AI Design Framework) for their meaningful assistance in refining concepts, improving clarity, and strengthening the overall quality of this work. Thank you to Yuji Hirose for the httplib.h MIT sourcecode.

I want to extend sincere appreciation to Marcel Krüger, Independent Theoretical Physics Researcher, for his friendship, his sharp theoretical insight, and his role in shaping the Wave Native Network alongside me. While AILEE CORE is a software architecture built from the ground up within this repository, the conceptual clarity and physics‑driven intuition Marcel brought to our discussions helped refine the final form of WNN and strengthen the direction of this project. His willingness to offer support, review ideas, and stand with me throughout this journey means more than a simple credit line can express. Thank you, Marcel, for being part of the path that led here.

---

## AILEE Core Heartbeat System (Protocol Runtime)

The AILEE Core Heartbeat System integrates the Wave Native Network (WNN) with the deterministic L2 execution engine. This forms a deterministic heartbeat loop that orchestrates state transitions without reliance on local wall-clock timers.

The deterministic heartbeat loop operates as follows:
- **Wave‑phase rollover → heartbeat:** The system listens for a mathematically deterministic wave‑phase boundary from the WNN. When crossed, it triggers a protocol heartbeat.
- **Heartbeat → epoch execution:** The heartbeat triggers the epoch scheduler to deterministically advance the current epoch and schedule its execution.
- **Epoch execution → new state root:** The deterministic runtime processes the epoch, computing a mathematically reproducible L2 state root.
- **State root → Bitcoin anchoring:** The finalized state root is formatted as a verifiable anchor commitment, ready to be submitted to the Bitcoin L1.

### Deterministic Execution & Bitcoin-Anchored Recovery

The protocol is driven by a **governed heartbeat** that enforces strict **deterministic execution** at every step. This model guarantees that given the same frozen genesis state and sequence of wave-phase boundaries, any node will compute the exact same L2 state root, ensuring absolute **replay guarantees** across disparate environments. To secure this lineage, the runtime employs a **Bitcoin‑anchored recovery** strategy: each finalized state root is formatted as a verifiable Taproot or OP_RETURN commitment and settled to the Bitcoin L1. This establishes an immutable sequence of checkpoints, allowing trust-minimized, deterministic re-execution of the entire L2 state from genesis.

## Deterministic Guarantees Introduced in V13 (Heartbeat Release)

### 1. Deterministic Wave‑Phase Boundaries (Eliminating Local Timing Drift)

Traditional distributed systems rely on local machine clocks (std::chrono, OS timers, NTP), which drift, jitter, and create nondeterministic execution windows. This leads to inconsistent ordering, ambiguous event boundaries, replay instability, and timing‑driven state divergence.

**V13 Heartbeat Shift:**  
AILEE Core replaces internal wall‑clock timing with deterministic **WaveState phase‑coherence boundaries**, providing:

- stable phase rollover triggers  
- deterministic epoch boundaries  
- reproducible timing transitions  
- architecture‑independent execution triggers  

Epochs advance **only** on wave‑phase rollover, ensuring stable, collision‑resistant coordination.  
This is a protocol‑internal timing upgrade, not an internet‑wide change.

---

### 2. Canonical Deterministic Execution Replays

Distributed systems often diverge due to floating‑point drift, scheduler nondeterminism, and inconsistent ordering. Even identical event sequences can yield different results.

**V13 Heartbeat Shift:**  
The Heartbeat loop enforces **bit‑for‑bit reproducible execution**:

- deterministic wave‑phase trigger  
- deterministic epoch scheduler  
- deterministic `run_epoch()` execution  
- canonical L2 merkleized state root  
- deterministic Bitcoin‑ready anchor payload  

This produces a **globally reproducible execution lineage**, independent of CPU architecture, OS scheduling, or hardware timing.

---

### 3. Bitcoin‑Anchored Recovery & Deterministic Rebuild Pathways

Systems suffering hardware failure, regional outage, corruption, or catastrophic loss rarely achieve trust‑minimized recovery.

**V13 Heartbeat Shift:**  
Each heartbeat emits:

- a canonical state root  
- a deterministic anchor payload  
- a verifiable OP_RETURN/Taproot‑ready commitment  

This provides:

- immutable recovery checkpoints  
- trust‑minimized replay paths  
- deterministic re‑execution from genesis  
- automatic reorg detection  

Nodes can rebuild state with **zero ambiguity** by verifying anchored roots and replaying epochs deterministically.  
This is a protocol‑level guarantee built on Bitcoin’s immutability.

---

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.


