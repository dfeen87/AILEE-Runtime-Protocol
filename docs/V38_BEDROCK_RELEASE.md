# AILEE 38.0.0 BEDROCK Release

## Release rationale and architecture

Version 38.0.0 establishes a new engineering baseline after a bottom-up review of
the runtime, trust-layer pipeline, tests, build metadata, and CI. The existing
architecture remains intentionally preserved: C++ owns deterministic runtime
behavior, FastAPI exposes service interfaces, TypeScript supplies communication-clock
and trust-layer mirrors, and CMake remains the native build and test registry.

The major increment follows the repository's stated Semantic Versioning policy:
malformed-input handling and trust-decision behavior are intentionally incompatible,
although valid interfaces were not redesigned.

## Explicit invariants and corrected weaknesses

1. **Governance is bound to authoritative posture.** The integrated runtime replaces
   duplicated caller-provided posture and coherence factors with values computed from
   the same telemetry evaluation. Risky telemetry cannot be paired with a separate
   claim of safe posture to obtain approval.
2. **Trust evidence is finite and in-domain.** Posture must be finite and in `[0, 10]`;
   coherence must be finite and in `[0, 1]`; recursion roots must be 32-byte
   hexadecimal values; and governance thresholds must be finite and supported.
3. **Malformed telemetry cannot become nominal telemetry.** Any non-finite floating
   telemetry deterministically returns maximum risk, zero coherence, zero confidence,
   and a stress regime. REST request models reject such values before evaluation.
4. **Ledger identity is content-derived.** Recording always computes the identifier
   after parent and timestamp resolution. Verification recomputes it, validates numeric
   domains, and checks the canonical genesis link and every subsequent parent link.
5. **Rejected transitions remain non-mutating.** The compartment state machine already
   validates transitions while holding its mutex and mutates only after validation;
   that sound behavior was retained.

## Regression proof and CI enforcement

Regression tests cover non-finite and malformed approval evidence, invalid
constitutions, ledger identifier injection, ledger content mutation, authoritative
posture binding, and non-finite telemetry. Existing transition, determinism, stress,
anchoring, replay, policy, energy, API, and communication-clock tests remain intact.

CI now builds all registered CMake test executables and runs CTest instead of only
invoking `ailee_tests`. It also runs Python tests, TypeScript type checking and Jest,
and the release-metadata consistency check. Normal and heartbeat builds both execute
the registered CTest suite.

## Compatibility and newly rejected behavior

- Valid finite telemetry and correctly formed SHA-256 recursion roots retain their
  evaluation behavior.
- The `/v37` REST path remains available and reports release 38.0.0.
- Short/non-hex roots, NaN/infinity, out-of-domain governance values, unsafe
  constitutions, caller-selected ledger IDs, and content whose identifier no longer
  matches are rejected.
- Runtime callers supplying posture values inconsistent with accompanying telemetry
  receive the decision derived from authoritative telemetry.

## Investigated risks and external boundaries

The review traced configuration into API initialization; telemetry through posture,
interpretation, approval, and ALCOA recording; compartment transitions under locking;
ledger linkage and hashing; CMake test registration; Python/Jest discovery; and active
version strings. Optional RocksDB, ZeroMQ, libp2p, Rust prover, and energy targets keep
their existing build-time detection or switches.

No related repository was present in the workspace, so external trust-layer
compatibility is not claimed. Broader concurrency of the in-memory ALCOA ledger,
durable transactional persistence across process failure, and cryptographic signature
verification remain risks outside this focused correction; approval inputs currently
report signature-verification results rather than performing key verification.

This release improves software invariants; it is not Bitcoin consensus review,
cryptographic certification, operational security certification, financial approval,
or deployment certification. Production use still requires independent threat
modeling, key-management review, adversarial/load testing, storage recovery testing,
integration against the selected Bitcoin node and optional dependencies,
reproducible-build verification, and domain-specific review.

The architecture was preserved where sound. The guarantees underneath it were
strengthened.
