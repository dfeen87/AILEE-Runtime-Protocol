# Changelog

All notable changes to this project are documented here.

Note: v1.1.0 was intentionally skipped to align versioning with capability boundaries introduced in v1.2.0.

## [Unreleased]

### Changed
- None.

### Rationale
This change improves alignment between documentation and current implementation status.

---

## v38.0.0 — BEDROCK Engineering Baseline

- Bound governance approval to posture and coherence values produced by the runtime's authoritative telemetry evaluation.
- Made governance evidence fail closed for non-finite/out-of-domain numbers, malformed recursion roots, and invalid constitutions.
- Made ALCOA entry identifiers content-derived and added content-hash and genesis-link verification.
- Made non-finite posture telemetry deterministically produce maximum risk and zero coherence.
- Added regression coverage for malformed evidence, telemetry, ledger tampering, invalid governance configuration, and cross-component posture binding.
- Expanded CI from one directly invoked C++ binary to CTest, Python, TypeScript, and release-metadata checks.
- Preserved the layered architecture and `/v37` REST compatibility route. The major milestone reflects stricter behavioral contracts, not a wholesale API redesign.

See `docs/V38_BEDROCK_RELEASE.md` for the complete report and validation boundaries.

---

## v37.1.0 — Repository Organization

### Changed
- Synchronized package, runtime, API, web, and citation metadata at version 37.1.0.
- Collected the V12 reproducibility bundle under `artifacts/v12/`.
- Moved supplemental documentation and retired build-repair material out of the repository root.

---

## v5.2.0 — Security Hardening & Architectural Uplift

### Security
- Fixed timing-attack vulnerability in API key comparison (use `hmac.compare_digest`)
- API key no longer logged in full to console (prefix-only logging)
- Added hexTx input validation in Bitcoin RPC client (non-empty, hex chars only, ≤ 400KB)
- SSL configuration now fails-closed instead of silently downgrading to HTTP
- CORS wildcard produces a warning in production mode
- Mempool deduplication prevents duplicate transaction flooding
- EnergyProof signature verification defaults to fail-closed (TODO: implement ECDSA)
- Added null-pointer guard to expression evaluator EvalContext

### Robustness
- Config YAML parser validates required fields before accessing (prevents uncaught exceptions)
- NetFlow bandwidth allocation clamped to `max(0, value)` to prevent negative values
- Config hot-reload circuit breaker now has a programmatic `reset()` API

### Architecture
- Extracted shared SHA-256 utility to `include/crypto_utils.h` to eliminate duplicate implementations
- Removed duplicate `relayBandwidth()` inline definition from header (kept cpp implementation with tunnel-mode penalty logic)
- Cleaned up dead `running_` member from `AILEEWebServer` header (was shadowed by `Impl::running_`)
- Database layer variable renamed from `_db_pool` to `_db_connection` to accurately reflect single-connection model

### Maintainability
- Standardized all third-party JSON include paths to `nlohmann/json.hpp`
- All version numbers synchronized to 5.2.0 across CMakeLists.txt, pyproject.toml, CITATION.cff, api/config.py, examples/WebServerDemo.cpp
- Added `__all__` to `api/routers/__init__.py` for explicit module listing
- CHANGELOG updated with all previously undocumented releases
- `scripts/check-metadata.sh` updated to verify version in `examples/WebServerDemo.cpp`

---

## v2.0.0 — AmbientAI Core & Energy Proof

- Added `AmbientAI-Core.cpp` with Byzantine Fault Tolerant consensus mechanism
- Added verifiable energy telemetry with IoT oracle integration (`EnergyProof`)
- Advanced token economics with smart contract simulation
- Multi-signature validation for distributed decisions
- Real-time system health diagnostics

---

## v3.0.0 — NetFlow & DAO Governance

- Added `AILEE_NetFlow.cpp`: Hybrid Layer-2 decentralized internet relay engine
- ZK Proof-of-Bandwidth tokenized incentives
- Onion and WireGuard tunnel modes with bandwidth accounting
- Added `ailee_dao_governance.h`: DAO governance model for protocol upgrades
- Config hot-reload with circuit-breaker and exponential backoff

---

## v4.0.0 — Mempool, BlockProducer & Reorg Detection

- Added `Mempool.h`/`Mempool.cpp`: Thread-safe L2 transaction queue
- Added `BlockProducer.cpp`: L2 block production from mempool
- Added `ReorgDetector.h`: Bitcoin chain reorganization detection
- Added `AILEEWebServer.cpp`: Embedded REST API (httplib) with CORS and API key auth
- Added `WebServerDemo.cpp` example

---

## v1.2.1 — Build & Test Hardening
- Fixed unit test source paths for CMake discovery
- Added FetchContent fallback for GoogleTest when not installed
- Added CI workflow to build and run tests on Ubuntu

---

## v1.2.0 — AILEE Observational Adapter Layer
- Added optional, read-only AILEE adapters:
  - Mempool state
  - Network topology
  - Energy efficiency (AILEE η)
- No changes to consensus behavior
- No transaction routing or submission changes
- Bitcoin adapter remains authoritative
- Telemetry enrichment only

---

## v1.1.0 — Verification & Systems Integrity
- Added formal verification and falsifiability boundaries
- Clarified system assumptions and non-goals
- No code changes

## v1.0.0 — Initial Public Release
- Core AILEE framework
- Multi-chain adapter architecture
- Orchestration and telemetry foundations
