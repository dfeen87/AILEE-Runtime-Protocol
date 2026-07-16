# AILEE Protocol Core for Bitcoin: **Ambient AI + Wave Native Network. Bitcoin Layer-2 Orchestration Framework**

*Building Intelligent, Verifiable, and Sustainable Bitcoin Infrastructure*

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-blue.svg)](https://cmake.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-Python-green.svg)](https://fastapi.tiangolo.com/)
[![Bitcoin](https://img.shields.io/badge/Bitcoin-Layer--2-orange.svg)](https://bitcoin.org/)
[![CI](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/actions/workflows/ci.yml/badge.svg)](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/actions/workflows/ci.yml)


**[Documentation](docs/)** | **[Quick Start](#-quick-start)** | **[API Reference](API_QUICKSTART.md)** | **[Architecture](docs/ARCHITECTURE_CONCEPTUAL.md)** | **[Contributing](docs/CONTRIBUTING.md)**

---

Current Version number: 33.4

## 📑 Table of Contents
- [🎯 Overview](#-overview)
- [⚠️ Protocol Status & Safety Notice](#-protocol-status-safety-notice)
- [Building the AILEE-Runtime-Protocol (Full Terminal Guide + External Module Requirements)](#building-the-ailee-runtime-protocol-full-terminal-guide--external-module-requirements)
  - [Step 1 — Clone the repository](#step-1-clone-the-repository)
  - [Step 2 — Create a build directory](#step-2-create-a-build-directory)
  - [Step 3 — Configure the project](#step-3-configure-the-project)
  - [Step 4 — Build the runtime](#step-4-build-the-runtime)
  - [Step 5 — Run the runtime](#step-5-run-the-runtime)
- [Executable Name & System Entrypoint](#executable-name-system-entrypoint)
  - [`./protocol-node`](#protocol-node)
- [Understanding Placeholder Modules](#understanding-placeholder-modules)
- [Required External Modules (You Must Build These)](#required-external-modules-you-must-build-these)
  - [Required External Files / Modules](#required-external-files-modules)
- [Designing Your Broadcast Subsystem](#designing-your-broadcast-subsystem)
- [Designing Your WebServer Subsystem](#designing-your-webserver-subsystem)
- [Unified Clock Synchronization Through Bitcoin](#unified-clock-synchronization-through-bitcoin)
- [Order‑of‑Operations in the Runtime](#orderofoperations-in-the-runtime)
- [Why External Modules Are Not Included](#why-external-modules-are-not-included)
- [Summary](#summary)
- [Capabilities for Cloners and Integrators](#capabilities-for-cloners-and-integrators)
- [Files Required for Real‑Time External Communication](#files-required-for-realtime-external-communication)
- [Deterministic Release & Wave Native Network Integration](#deterministic-release-wave-native-network-integration)
  - [Wave Native Network (WNN) Integration](#wave-native-network-wnn-integration)
  - [V12 Reproducibility Package](#v12-reproducibility-package)
  - [Canonical Artifacts & Epoch Roots](#canonical-artifacts-epoch-roots)
  - [V12 Verification Script (`verifyv12.sh`)](#v12-verification-script-verifyv12sh)
  - [Why This Matters](#why-this-matters)
- [✨ Some other Key Features](#-some-other-key-features)
  - [🛡️ Reorg Detection](#-reorg-detection)
  - [🧠 Ambient AI Orchestration](#-ambient-ai-orchestration)
  - [⛓️ Bitcoin Layer-2 Infrastructure](#-bitcoin-layer-2-infrastructure)
  - [🌐 Web & API Integration](#-web-api-integration)
- [🧪 Testing & Validation](#-testing-validation)
- [🏗️ Architecture](#-architecture)
  - [The Three-Pillar Architecture](#the-three-pillar-architecture)
  - [Formalized Temporal Pipeline (V32 Protocol Specification)](#formalized-temporal-pipeline-v32-protocol-specification)
  - [Orchestration Pipeline Overview](#orchestration-pipeline-overview)
  - [Design Principles & Protocol Guarantees](#design-principles-protocol-guarantees)
- [🔐 Security Model](#-security-model)
  - [✅ Explicit Federated Model](#-explicit-federated-model)
- [⚠️ What This Is NOT](#-what-this-is-not)
- [🛠️ Technology Stack](#-technology-stack)
  - [Core Infrastructure (C++)](#core-infrastructure-c)
  - [REST API (Python)](#rest-api-python)
- [🌍 Community & Support](#-community-support)
  - [Get Help](#get-help)
  - [Citation](#citation)
- [Acknowledgments](#acknowledgments)
- [AILEE Core Heartbeat System (Protocol Runtime)](#ailee-core-heartbeat-system-protocol-runtime)
  - [Deterministic Execution & Bitcoin-Anchored Recovery](#deterministic-execution-bitcoin-anchored-recovery)
- [Deterministic Guarantees Introduced in V13 (Heartbeat Release)](#deterministic-guarantees-introduced-in-v13-heartbeat-release)
  - [1. Deterministic Wave‑Phase Boundaries (Eliminating Local Timing Drift)](#1-deterministic-wavephase-boundaries-eliminating-local-timing-drift)
  - [2. Canonical Deterministic Execution Replays](#2-canonical-deterministic-execution-replays)
  - [3. Bitcoin‑Anchored Recovery & Deterministic Rebuild Pathways](#3-bitcoinanchored-recovery-deterministic-rebuild-pathways)
- [1. Recursive Zero‑Knowledge State Compression (V33 Recursion Layer)](#1-recursive-zeroknowledge-state-compression-v33-recursion-layer)
- [2. Deterministic Taproot Anchoring Engine (V33 Anchor Layer)](#2-deterministic-taproot-anchoring-engine-v33-anchor-layer)
- [3. Isla Mode Autonomous Heuristic Engine (V33 Isla Layer)](#3-isla-mode-autonomous-heuristic-engine-v33-isla-layer)
- [📄 License](#-license)
- [🙏 Closing Reflection](#-closing-reflection)

---


## 🎯 Overview

**AILEE. Sometimes referred to as AILEE Core** is a Bitcoin Layer‑2 orchestration and verification framework that provides deterministic off‑chain computation, state‑integrity validation, and federated learning coordination. It extends Bitcoin’s capabilities through a recovery‑first design and **ambient intelligence** that remains strictly advisory, ensuring temporal insight without altering Bitcoin’s consensus rules.

> AILEE‑Core operates as a governed execution environment: the **Governor** establishes deterministic heartbeat, the **Auditor** enforces temporal coherence, the **Energy Resilience Layer** stabilizes runtime behavior, and **IAO** provides forward‑looking orchestration while preserving strict determinism.

> *With the core components established, it is essential to understand the current maturity and safe deployment boundaries of the AILEE protocol.* 🛡️

## ⚠️ Protocol Status & Safety Notice

The **AILEE‑Runtime‑Protocol** (formerly AILEE‑Core) has evolved into a **deterministic, reproducible internet protocol that synchronizes with and can touch the Bitcoin Main Network by communications**, functioning as a **Layer‑2 temporal alignment system** for unified clock speeds, reproducible execution, and deterministic state propagation.  

The software verifies execution paths, canonical state roots, and integrates with the Wave Native Network for temporal resonance and epoch‑level synchronization.


Version 12 introduces:

- deterministic execution  
- reproducible state transitions  
- frozen genesis  
- canonical build + receipt hashes  
- verified epoch roots (0–4)  
- full deterministic re‑execution path  
- automated verification script  
- unified clock synchronization through Bitcoin  
- reproducible temporal ordering  
- deterministic broadcast + webserver hooks (user‑implemented)  

These guarantees establish the AILEE‑Runtime‑Protocol as a **verifiable protocol**, not a prototype.

All core components in this repository are **real implementations** — no stubs, mocks, placeholder cryptographic primitives, or simulated networking. The deterministic runtime, reproducibility package, and Wave Native Network integration operate as fully functional, verifiable subsystems.

However, before mainnet fund custody or production‑grade deployment, several hardening steps remain:

- ZK proof system must be upgraded to audited circuits (Halo2, PLONK, Groth16)  
- Signature verification requires formal review  
- Networking layers must undergo security auditing  
- External broadcast modules must be implemented by the adopting entity  
- External WebServer modules must be implemented by the adopting entity  
- Mainnet‑touching RPC logic must be customized per organization  
- Third‑party audits are required for any production deployment  

The AILEE‑Runtime‑Protocol V12 is a **protocol‑grade deterministic release**, but **not yet a mainnet‑ready financial system**.

> “AILEE‑Runtime‑Protocol is not a prototype anymore — but it is not a custodial mainnet system.”

---

> *Having established the protocol's guarantees and safety guidelines, the next step is getting the system running in your own environment.* 🛠️

---

# **Building the AILEE‑Runtime‑Protocol (Full Terminal Guide + External Module Requirements)**

The **AILEE‑Runtime‑Protocol** is designed as a **deterministic execution substrate** that can be integrated into corporate systems, independent research environments, or distributed infrastructures requiring reproducible runtime behavior. This repository provides the **core deterministic runtime**, but intentionally **does not** ship production‑grade broadcast modules or webserver implementations. These components must be designed by the entity adopting the protocol.

This section explains:

- how to build the runtime from a terminal  
- how to integrate your own broadcast + webserver modules  
- which files act as placeholders  
- which files must be created for real‑time external communication  
- how unified clock synchronization works  
- how Bitcoin’s internal timing influences order‑of‑operations  
- how to extend the protocol for mainnet‑touching systems  

---

## **1. Building the Runtime From Terminal**

The AILEE‑Runtime‑Protocol uses a standard CMake‑based build pipeline.  
To build the runtime:

### **Step 1 — Clone the repository**
```
git clone https://github.com/dfeen87/AILEE-Runtime-Protocol.git
cd AILEE-Runtime-Protocol
```

### **Step 2 — Create a build directory**
```
mkdir build
cd build
```

### **Step 3 — Configure the project**
```
cmake ..
```

This step resolves:

- deterministic runtime includes  
- internal protocol modules  
- placeholder broadcast + webserver references  
- unified clock sync subsystem  
- reproducible build flags  

### **Step 4 — Build the runtime**
```
cmake --build .
```

This produces the core executable for the deterministic runtime.

### **Step 5 — Run the runtime**

Tip: Make sure your in the build folder, then proceed to prompt.

```
./runtime
```

This launches the deterministic execution engine.  

**Note:** Without custom broadcast + webserver modules, the runtime will operate in placeholder mode.

---

> *Once compiled, the protocol's system bootloader takes over. How you invoke this executable defines the role of your node.* 🚀

---

## **Executable Name & System Entrypoint**

The AILEE‑Runtime‑Protocol ships with a single system entrypoint — the executable that launches the entire deterministic protocol environment. While many runtimes use a generic name like `./runtime`, this protocol is **multi‑module**, **deterministic**, and **system‑level**, meaning the executable represents more than a simple runtime loop.

Cloners may rename the executable to match their infrastructure, but the recommended name is:

### **`./protocol-node`**

This name reflects the true role of the entrypoint:

- it initializes the deterministic runtime  
- it activates unified clock synchronization  
- it loads reproducible state transitions  
- it prepares Wave Native Network hooks  
- it connects to user‑implemented broadcast modules  
- it connects to user‑implemented webserver modules  
- it anchors temporal ordering to Bitcoin  
- it executes the protocol as a full node in a deterministic system  

In other words, the executable is **not** just a runtime — it is the **system launcher** for the entire protocol.

Entities may rename the executable to suit their environment:

- **`./node`** — minimal, distributed‑systems style  
- **`./engine`** — emphasizes deterministic execution  
- **`./protocol`** — emphasizes the system identity  
- **`./daemon`** — for long‑running background processes  
- **`./deterministic-node`** — highlights reproducibility  
- **`./multi-runtime`** — highlights multi‑use architecture  

Regardless of the chosen name, the executable represents the **full protocol boot sequence**, not a single subsystem. Cloners should select a name that matches their operational model, infrastructure, and deployment environment.

---

> *While the core deterministic engine is fully functional, AILEE is designed for enterprise integration. Certain components are intentionally left open for customization.* 🧩

---

## **Understanding Placeholder Modules**

The repository intentionally includes **placeholder references** for:

- broadcast subsystem  
- webserver routing  
- external RPC communication  
- mainnet‑touching operations  
- unified clock sync endpoints  

These placeholders exist so that:

- corporations  
- independent developers  
- research institutions  
- protocol engineers  

can design their own modules tailored to their infrastructure.

The runtime is **multi‑use**, meaning it can be embedded into:

- financial systems  
- distributed compute environments  
- temporal synchronization frameworks  
- deterministic replay engines  
- blockchain‑anchored applications  

But **your organization must implement the external communication layer**.

---

> *To bridge the gap between the deterministic L2 substrate and the outside world, you will need to implement specific external handlers tailored to your architecture.* 🏗️

---

## **Required External Modules (You Must Build These)**

Below is the list of modules **not provided** by the repo, because they must be customized by each entity.

### **Required External Files / Modules**
- **BroadcastHandler.cpp** — your logic for sending raw transactions  
- **BroadcastRouter.cpp** — your routing rules for broadcast endpoints  
- **BroadcastRPC.cpp** — your RPC interface to Bitcoin Core  
- **WebServer.cpp** — your HTTP server implementation  
- **WebRoutes.cpp** — your custom API endpoints  
- **ExternalConfig.json** — your environment‑specific configuration  
- **TemporalSyncAdapter.cpp** — your integration with Bitcoin timestamps  
- **MainnetAdapter.cpp** — your mainnet‑touching logic  
- **UnifiedClockClient.cpp** — your clock sync client for distributed nodes  

These files are **not** included because:

- every corporate environment has different RPC credentials  
- every independent developer uses different infrastructure  
- every institution has different security requirements  
- every mainnet‑touching system must be audited internally  

The runtime provides the **deterministic substrate**, not the external communication layer.

---

> *One of the most critical external modules is the broadcast subsystem, responsible for anchoring state directly to the Bitcoin network.* 📡

---

## **Designing Your Broadcast Subsystem**

Your broadcast subsystem must:

- accept raw transaction hex  
- validate payload structure  
- communicate with Bitcoin Core  
- return txid or error  
- log deterministic results  
- integrate with your webserver  

Example responsibilities:

- `sendrawtransaction` RPC  
- mempool inspection  
- error propagation  
- deterministic logging  
- temporal alignment with Bitcoin blocks  

The repo provides **placeholder references**, but **you must implement the logic**.

---

> *Alongside Bitcoin network integration, your node will likely need to expose APIs to users and internal services.* 🌐

---

## **Designing Your WebServer Subsystem**

Your webserver must:

- expose your custom API endpoints  
- route broadcast requests  
- expose health checks  
- expose temporal sync endpoints  
- integrate with your internal systems  

The runtime will call into your webserver, but does not ship one.

This ensures:

- corporate systems can use their own HTTP stack  
- independent developers can use lightweight servers  
- institutions can use hardened infrastructure  

---

> *To maintain strict determinism across all these interacting subsystems, AILEE utilizes Bitcoin itself as a universal timing anchor.* ⏱️

---

## **Unified Clock Synchronization Through Bitcoin**

The runtime includes a subsystem that aligns execution with **Bitcoin’s internal timing**, using:

- block timestamps  
- mempool propagation timing  
- ZMQ notifications  
- RPC block headers  
- consensus‑verified temporal signals  

This creates a **global unified clock** that:

- eliminates OS timer drift  
- synchronizes distributed nodes  
- anchors execution to Bitcoin’s order‑of‑operations  
- ensures deterministic replay across machines  

Your external modules must integrate with this subsystem.

---

> *With time synchronized and external modules attached, the protocol boot sequence follows a highly structured flow to ensure absolute reproducibility.* 🔄

---

## **Order‑of‑Operations in the Runtime**

The deterministic runtime follows a strict sequence:

1. Load configuration  
2. Initialize unified clock sync  
3. Initialize deterministic executor  
4. Load placeholder broadcast + webserver references  
5. Wait for external module integration  
6. Execute deterministic tasks  
7. Anchor temporal events to Bitcoin  
8. Replay or broadcast based on your custom modules  

This order ensures reproducibility across:

- machines  
- architectures  
- environments  
- distributed systems  

---

> *You might wonder why these critical modules aren't shipped out-of-the-box. The answer lies in the protocol's multi-use, flexible design philosophy.* 💡

---

## **Why External Modules Are Not Included**

The protocol is designed for **multi‑use environments**, meaning:

- corporations  
- independent developers  
- research labs  
- distributed compute networks  

all have different requirements for:

- security  
- RPC credentials  
- infrastructure  
- mainnet access  
- temporal sync  
- API design  

Therefore, the repo ships **only the deterministic runtime**, not the external communication layer.

This is intentional.

---

> *To distill the architecture: AILEE provides the deterministic core, while you provide the interfaces.* 📝

---

## **Summary**

The **AILEE‑Runtime‑Protocol** provides:

- deterministic execution  
- unified clock sync  
- reproducible builds  
- multi‑use architecture  

You must provide:

- broadcast subsystem  
- webserver subsystem  
- external RPC logic  
- mainnet‑touching modules  
- temporal sync adapters  
- unified clock clients

When both your files and this repos MIT Licensed files, cloners can communicate freely with hardware and internet technologies, your entities will be able to do a variety of things.

---

> *By combining the AILEE deterministic core with your custom external modules, a vast array of powerful use cases becomes possible.* 🚀

---

## **Capabilities for Cloners and Integrators**

The **AILEE‑Runtime‑Protocol** provides a deterministic execution substrate that organizations, independent developers, and research teams can extend with their own broadcast, webserver, and external communication modules. While this repository does not ship production‑ready external interfaces, the runtime enables a wide range of advanced capabilities once those modules are implemented.

Below is a comprehensive list of what cloners can achieve by integrating their own systems with the runtime:

- **Convert Gold to Bitcoin** —  
  Entities can design custom financial modules that anchor commodity conversion logic to Bitcoin’s deterministic timing. The runtime ensures reproducible execution for conversion algorithms, audit trails, and temporal verification.

- **Save energy and output more TPS** —  
  By using deterministic execution and unified clock synchronization, systems can reduce wasted cycles, eliminate drift, and optimize throughput. This results in lower energy consumption and higher transaction processing speeds when integrated with custom broadcast layers.

- **Participate in Wave Native Network** —  
  The runtime can be extended to join temporal‑synchronization networks such as Wave Native Network. This allows nodes to align phase‑identity, resonance timing, and deterministic epochs across distributed systems.

- **Run curl commands for latency testing** —  
  Developers can integrate custom webserver modules that expose endpoints for latency measurement, clock verification, and deterministic logging. Using curl, entities can test:
  - request/response timing  
  - unified clock drift  
  - broadcast latency  
  - temporal alignment with Bitcoin  

- **Unified Clock Synchronization** —  
  Systems can anchor their internal clocks to Bitcoin’s consensus‑verified timestamps. This eliminates OS drift and ensures reproducible execution across machines, architectures, and environments.

- **Deterministic Replay** —  
  The runtime guarantees identical results across nodes, enabling deterministic replay for:
  - audits  
  - simulations  
  - distributed compute  
  - financial operations  

- **Custom Broadcast Subsystems** —  
  Entities can design their own broadcast logic to:
  - send raw transactions  
  - integrate with Bitcoin Core  
  - propagate mempool events  
  - log deterministic broadcast outcomes  

- **Custom WebServer Implementations** —  
  Organizations can build their own HTTP servers to:
  - expose API endpoints  
  - route broadcast requests  
  - expose temporal sync endpoints  
  - integrate with internal systems  

- **Mainnet‑Touching Operations** —  
  Once external modules are implemented, the runtime can:
  - anchor events to Bitcoin  
  - read block headers  
  - verify timestamps  
  - propagate transactions  
  - synchronize distributed nodes  

- **Distributed System Coordination** —  
  The deterministic runtime can serve as a coordination layer for:
  - multi‑node compute  
  - temporal alignment  
  - reproducible workflows  
  - cross‑infrastructure synchronization  

- **Corporate, Institutional, or Independent Customization** —  
  Because the repo ships placeholder modules, each entity can design:
  - their own RPC credentials  
  - their own broadcast logic  
  - their own webserver stack  
  - their own security model  
  - their own temporal sync adapters  

This ensures the protocol can be adopted across industries without forcing a single architecture.

---

> *For quick reference, here is the exact checklist of interface files you will need to construct for a production deployment.* 📋

---

## **Files Required for Real‑Time External Communication**

Cloners must implement the following files to enable real‑time communication, mainnet interaction, and unified clock synchronization:

- **BroadcastHandler.cpp** — raw transaction handling  
- **BroadcastRouter.cpp** — routing logic  
- **BroadcastRPC.cpp** — Bitcoin Core RPC integration  
- **WebServer.cpp** — HTTP server implementation  
- **WebRoutes.cpp** — API endpoint definitions  
- **TemporalSyncAdapter.cpp** — Bitcoin timestamp integration  
- **UnifiedClockClient.cpp** — distributed clock sync  
- **MainnetAdapter.cpp** — mainnet‑touching logic  
- **ExternalConfig.json** — environment‑specific configuration  

These files are intentionally **not** included in the repository.  
They must be designed by the entity adopting the protocol.

---

> *Underpinning all of these capabilities is our most significant advancement: true bit-for-bit determinism and Wave Native synchronization.* 🌊

---

## Deterministic Release & Wave Native Network Integration

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

> *Beyond reproducibility and temporal stability, the AILEE Core ecosystem offers several advanced built-in functionalities out of the box.* ✨

---

## ✨ Some other Key Features

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

> *A deterministic system is only as strong as its tests. We place immense emphasis on rigorous verification.* 🧪

---

## 🧪 Testing & Validation

**Confidence requires verification.**  
The AILEE‑Runtime‑Protocol is engineered for deterministic, reproducible execution, but any system that *touches Bitcoin Mainnet* or participates in temporal synchronization networks must undergo rigorous, continuous validation. Determinism is only meaningful when it is **proven under stress**, **proven under adversarial conditions**, and **proven across machines**.

Before any real‑world deployment—especially in environments involving financial operations, commodity conversion, or temporal coordination—we strongly recommend the following validation steps:

1. **Unit & Integration Testing** —  
   Execute the full CMake/CTest suite (`ctest`) to validate deterministic behavior across all internal modules. This includes the Deterministic Executor, Unified Clock Sync subsystem, Reorg Detector, Adapter Registry, and reproducible state‑transition logic.

2. **Adversarial Simulation** —  
   Use the `PerformanceSimulator` to stress‑test the protocol under extreme load, malformed inputs, rapid temporal shifts, and simulated adversarial conditions. This ensures the runtime maintains deterministic guarantees even when external modules behave unpredictably.

3. **Reorg Simulation** —  
   Connect the protocol to a Bitcoin regtest environment and intentionally invalidate blocks to trigger the Reorg Detector. This validates:
   - temporal rollback handling  
   - deterministic replay  
   - state‑root consistency  
   - unified clock realignment  

4. **Latency & Clock Drift Testing** —  
   Once a cloner implements their own WebServer module, use curl to measure:
   - request/response latency  
   - unified clock drift  
   - temporal propagation delays  
   - deterministic logging accuracy  

5. **Broadcast Path Validation** —  
   After implementing custom broadcast modules, test raw transaction propagation, RPC timing, mempool behavior, and deterministic broadcast logging. This ensures your external modules behave consistently with the runtime’s deterministic expectations.

6. **Third‑Party Audit** —  
   While the deterministic architecture is sound, any system interacting with Bitcoin Mainnet or handling financial operations should undergo independent review. This includes:
   - cryptographic audits  
   - RPC security audits  
   - networking audits  
   - temporal‑sync verification  
   - broadcast‑path hardening  

> **“Trust, but verify.”**  
> The core philosophy of Bitcoin—and the core philosophy of the AILEE‑Runtime‑Protocol.

---



> *To conceptualize how all these moving parts fit together, let's look at the high-level system architecture and temporal pipeline.* 🏗️

---

## 🏗️ Architecture

AILEE is designed as a deterministic Layer-2 protocol that enhances Bitcoin's capabilities without altering its base layer consensus.

### Formalized Temporal Pipeline (V32 Protocol Specification)

The strict deterministic operations of AILEE-Core run on a highly formalized sequential pipeline, enforcing how time, execution, and state coherence are handled across the network.

<div align="center">

```

┌────────────────────────────────────────────────────────────────────┐
│                     AILEE-RUNTIME-PROTOCOL NODE                    │
├────────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────┐  ┌───────────────────────────────┐  │
│  │ Deterministic Runtime    │  │ Unified Clock Synchronization │  │
│  │ Engine                   │  │ • Bitcoin Timestamp Anchoring │  │
│  └──────────────────────────┘  │ • Drift Elimination            │  │
│                                └───────────────────────────────┘  │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │ Deterministic Replay & Epoch Verification                    │  │
│  │ • Frozen Genesis                                             │  │
│  │ • Canonical Build Hashes                                     │  │
│  │ • Receipt Hash Verification                                  │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │ Wave Native Network Integration                              │  │
│  │ • Temporal Resonance                                         │  │
│  │ • Phase Alignment                                            │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │ External Modules (Implemented by Cloner)                     │  │
│  │ • Broadcast Subsystem                                        │  │
│  │ • WebServer / API Layer                                      │  │
│  │ • RPC Adapters                                               │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
                                ↕
           ┌───────────────────────────────────────────────┐
           │              BITCOIN MAINNET (L1)             │
           │        • Block Headers / Timestamps           │
           │        • ZMQ Notifications (Optional)         │
           │        • Mempool Timing (Optional)            │
           └───────────────────────────────────────────────┘

```

</div>

#### Temporal Architecture Components

As formalized in the V32 Protocol standard, the execution flow is strictly segmented:

- **V28 Governor (Ingress & Boundaries):** Establishes the absolute bounds of the protocol from fixed baselines. It provides deterministic parameter adjustment and protocol governance, preventing multi-epoch drift. Heartbeats are triggered purely by mathematical wave-phase rollovers.
- **V29 Auditor (Verification & Coherence):** Ensures multi-epoch temporal coherence through strict, reproducible metric validations. It mathematically scores state transitions over rolling windows to enforce stability.
- **V30 Energy Resilience Layer [ERL] (Stabilization):** Manages Byzantine node resilience and resource allocation deterministically. It penalizes unstable actors while ensuring protocol progression under adverse conditions.
- **V31 Intelligence‑Assisted Orchestration [IAO] (Projection & Execution):** Drives the high-level scheduling and execution of epochs. It integrates state transitions with deterministic ZK proofs and anchored commitments.

### Orchestration Pipeline Overview

The complete temporal architecture of AILEE‑Core functions as a cohesive pipeline (Governor → Auditor → ERL → IAO):
1. **Governor:** Sets deterministic thresholds and bounds for the protocol based strictly on wave-phase boundary triggers.
2. **Auditor:** Scrutinizes ongoing epochs against these bounds for temporal coherence and stability.
3. **ERL:** Evaluates node participation and enforces energy-efficient, resilient execution constraints based on Auditor feedback.
4. **IAO:** Orchestrates the final epoch execution, ensuring the transition yields a canonical state root ready for anchoring.

Within this sequence operates **Isla Mode** — a deterministic heuristic engine bridging the ERL and IAO. Isla Mode provides forward-looking, advisory temporal insight to optimize proof schedules, acting as a pure mathematical function of historical states without relying on non-reproducible external variables.

### Design Principles & Protocol Guarantees

- **Determinism:** Bit-for-bit reproducible execution across all architectures, stripping away local timing and scheduler ambiguity. Identical initial states and inputs yield identical outputs.
- **Temporal Coherence:** Anchoring network actions to globally synchronized, phase-coherent boundaries rather than local wall-clock timers.
- **Auditable Trust Assumptions:** Explicit, verifiable dependencies mapped to deterministic zero-knowledge proofs and cryptographic commitments.
- **Forward‑Looking Orchestration:** Autonomous, deterministic parameter tuning (Isla Mode) that adapts to changing network states without losing historical lineage.
- **Canonical Roots:** Every completed epoch produces a universally agreed-upon SHA-256 hash representing the complete L2 state.
- **Developer Tooling Suite:** V32 introduces strict verification tools (`build_verifier.py`, `epoch_inspector.py`, `state_root_comparator.py`) ensuring build hashes, receipt hashes, and replay paths are perfectly reproducible.


> *Of course, none of this infrastructure matters without a rigorous, federated security model protecting state transitions and L1 anchoring.* 🔐

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

The AILEE‑Runtime‑Protocol is secure to clone and implement because its core architecture is fully deterministic, reproducible, and self‑contained, with no hidden networking, no embedded private keys, and no custodial logic. All critical components—deterministic execution, unified clock synchronization, reproducible state transitions, canonical build hashing, epoch verification, and Wave Native Network integration—operate entirely within the local runtime and do not expose financial pathways or mainnet‑touching operations by default. External communication layers such as broadcast modules, RPC adapters, and WebServer endpoints are intentionally not included, ensuring that each entity (corporate, institutional, or independent) implements its own hardened interfaces, credentials, and security models. This separation makes the protocol safe to clone, audit, extend, and integrate, while preserving strict control over any mainnet‑related functionality. In short, the runtime is secure because it provides deterministic guarantees without assuming custody, leaving all sensitive operations to the cloner’s own infrastructure.

---

> *To avoid misconceptions, it is equally important to define what AILEE-Core explicitly does not attempt to be.* 🚫

---

## ⚠️ What This Is NOT

AILEE-Core is **NOT**:
- ❌ A replacement for Bitcoin
- ❌ A consensus-changing protocol
- ❌ A trustless rollup (it is a federated sidechain)

---

> *Powering this entire ecosystem is a robust, high-performance technology stack centered around modern C++ and Python.* 🛠️

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

> *Building this future is a collaborative effort. We welcome your contributions, feedback, and academic engagement.* 🤝

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

I want to extend sincere appreciation to Marcel Krüger, Independent Theoretical Physics Researcher, for his friendship, his H.L.V. (Helix Light Vortex) theoretical insight, and his role in in shaping the Wave Native Network alongside me, his work in WNN was crucial for a refrence and innovation to include in this Repository. The conceptual clarity and physics‑driven intuition Marcel brought to our discussions helped refine the final form of WNN with better math and strengthen the direction of this project. His willingness to offer support, review ideas, and stand with me throughout this journey means more than a simple credit line can express. Thank you, Marcel, for being part of the path that led here.

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

# **AILEE Core — V33 Protocol Upgrade**

AILEE Core V33 introduces a trilogy of deterministic protocol advancements that elevate the system from a high‑performance L2 into a continuous, Bitcoin‑anchored, self‑optimizing execution environment. These three features operate together as a unified architectural layer.

---

## **1. Recursive Zero‑Knowledge State Compression (V33 Recursion Layer)**  
AILEE now verifies each epoch’s proof *inside* the next epoch’s circuit, forming a continuous, self‑compressing chain of mathematical validity.  
This recursion layer introduces:

- A canonical **ZKRecursionBundle** for prior‑epoch proof artifacts  
- A dedicated **ZKRecursionManager** for continuity enforcement  
- Deterministic mock recursion hashing for reproducible testing  
- Full epoch‑to‑epoch linkage: state roots, epoch IDs, and public inputs  

**Outcome:**  
AILEE compresses its entire historical timeline into a single recursive proof, enabling instant node sync and drastically reducing Bitcoin L1 footprint.

---

## **2. Deterministic Taproot Anchoring Engine (V33 Anchor Layer)**  
V33 introduces a concrete, deterministic C++ engine for writing the canonical L2 state root directly into Bitcoin Taproot (P2TR).  
This anchor layer provides:

- Deterministic Schnorr multi‑signature aggregation  
- Canonical P2TR output construction  
- Script‑tree commitments for fallback paths  
- A formalized **AnchorCommit** object for anchor metadata  
- Zero randomness in transaction assembly, fee selection, or key aggregation  

**Outcome:**  
AILEE becomes Bitcoin‑native in practice, producing reproducible Taproot anchors that reflect the canonical L2 state with mathematical precision.

---

## **3. Isla Mode Autonomous Heuristic Engine (V33 Isla Layer)**  
Isla Mode is a deterministic feedback system that tunes orchestrator behavior based on multi‑epoch coherence, performance, and economic metrics.  
This heuristic layer includes:

- The **IslaModeEngine** for computing deterministic tuning decisions  
- Multi‑epoch metric windows for coherence, performance, and fee conditions  
- A canonical **IslaTuningDecision** struct (batch size, proof interval, worker allocation, anchor cadence)  
- A categorical **AnchorCadence** enum (`NORMAL`, `TIGHT`, `RELAXED`)  
- Integration with `IslaRuntimeOrchestrator` via `applyTuning()`  

**Outcome:**  
AILEE becomes self‑optimizing — adjusting batch size, proof timing, resource allocation, and anchor cadence through pure mathematical rules, without randomness or machine learning.

---

# **Unified Impact of V33**

Together, these three layers transform AILEE Core into a deterministic, continuous, Bitcoin‑anchored protocol:

- **Recursive history** → one proof for the entire timeline  
- **Taproot anchoring** → canonical state roots committed directly to Bitcoin  
- **Autonomous tuning** → stable, adaptive behavior under changing network conditions  

V33 is the first release where AILEE behaves like a single, coherent organism — compressing its past, anchoring its present, and optimizing its future through pure reproducible mathematics.

---

For questions please ask. Your input or curiosity to ask for clarity, will keep this repo running at full capacity. Issues or Discussions will be greatly be appreciated.

Welcome to email me at Dfeen87@Outlook.com

---

## AILEE Runtime Protocol Research Project

The AILEE Runtime Protocol Research Project is a scientific infrastructure organization dedicated to advancing deterministic execution, operator‑level reproducibility, and audit‑anchored computational science. Its core artifact — the AILEE Runtime Protocol (ARP) — provides a mathematically constrained execution substrate designed to eliminate nondeterminism across distributed, scientific, and computational workflows. AILEE‑Runtime Protocol is a system that uses Bitcoin Main Net for orchestration delivering deterministic off‑chain computation, verifiable state transitions, federated learning coordination, and Bitcoin‑anchored recovery guarantees, all operating under explicit and auditable trust assumptions, to help sync architectures with Governance.

The AILEE Runtime Protocol Research Project is built on a deterministic runtime architecture defined by three core pillars:

- **A Deterministic Execution Model** that enforces strict operator constraints to guarantee identical results across machines, environments, and time;
- **An Audit‑Anchored Runtime** that provides cryptographically verifiable, fully traceable execution paths; and
- **Operator‑Level Formalism**, which expresses computation as invariant‑bound operator sequences, enabling reproducibility at the mathematical rather than environmental level.

This clean‑slate substrate supports reproducible biomedical pipelines—including CRISPR simulations, computational biology workflows, biomanufacturing models, and GxP‑compliant diagnostics—directly aligning with NIH reproducibility initiatives. It also eliminates nondeterminism in distributed consensus simulations, multi‑node scientific workflows, HPC pipelines, and deterministic cloud execution, matching NSF reproducibility cyberinfrastructure priorities. Finally, AILEE enables exact replication of computational experiments, deterministic physics simulations, reproducible AI inference, and audit‑anchored scientific workflows, positioning it squarely within NSF’s Expeditions in Computing focus on next‑generation deterministic and clean‑slate runtime paradigms.

Looking for Partners and fellow researchers. Principle Architects, looking for kind good people to join me. If you are interested in this AILEE Protocol and want to help shape it and create new innovations and offerings, we build revisions. Every facet of the Protocol has a paper trail and every revision matters. I have strong work ethics, and I am looking for leads in Grant Opportunities. I want to also welcome leaders in software and internet technologies, on to the Board of this Project. In time we can make sure and reassure others of our important work to create documentation for Business entities and Founders that are focused on synchronicity in AI to Legacy computer systems.

I am honored that some of you reading this post is intrigued and considering outreach, it feels good. Thank you to everyone already engaging in this announcement of the AILEE-Runtime-Protocol Project.

---

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## 🙏 Closing Reflection

This project has been a long, disciplined journey — thousand of commits, countless late‑night breakthroughs, and a steady push toward clarity and determinism. Through every sprint, every setback, and every moment of insight, I’m grateful to the Lord for strength, endurance, and guidance.

> *“But those who hope in the Lord will renew their strength;  
> they will soar on wings like eagles;  
> they will run and not grow weary,  
> they will walk and not be faint.”* — Isaiah 40:31

Building AILEE‑Core has been more than engineering — it has been a reminder that perseverance, faith, and purpose can carry a person through complexity into understanding. Soli Deo gloria.

In God We Trust.

Thank you for your time in this repository. I hope my work will help others.

Best and kind regards,
Don Michael Feeney Jr.
