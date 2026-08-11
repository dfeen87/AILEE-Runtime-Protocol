# Contributing to AILEE-Core

Thank you for considering a contribution to **AILEE-Core (AI-Load Energy Efficiency)**.

AILEE-Core is a **Bitcoin-anchored Layer-2 orchestration, verification, and recovery framework**.  
It implements security-relevant infrastructure, including deterministic state snapshots,
Bitcoin-consumable anchor commitments, federated exit authorization, and offline verification tooling.

Contributions are welcome — but they are expected to meet the standards appropriate for
**distributed systems and Bitcoin-adjacent infrastructure**.

---

## Project Philosophy

AILEE-Core is built on a few non-negotiable principles:

- **Explicit trust assumptions**  
  No hidden security guarantees. No implied trustlessness.

- **Determinism over convenience**  
  Serialization, hashing, and state transitions are consensus-sensitive.

- **Auditability over opacity**  
  Third parties must be able to verify behavior without trusting a running node.

- **Bitcoin-native conservatism**  
  We prefer clarity, restraint, and recoverability over aggressive optimization.

We welcome skepticism, critique, and adversarial thinking.
We do **not** welcome hand-waving, hype, or security theater.

---

## Who Should Contribute

We welcome contributors with backgrounds in:

- Distributed systems engineering
- Bitcoin protocol and infrastructure
- Cryptography and applied verification
- Systems programming (C++ / Rust)
- Security review and threat modeling
- Documentation and technical writing

You do **not** need to agree with every design decision — but you must engage with them honestly.

---

## Contribution Areas

### 1. Layer-2 State & Verification

- Review deterministic snapshot logic and canonical serialization
- Improve state-root reproducibility guarantees
- Add regression vectors for `L2StateRoot` computation
- Extend offline verification tooling (`ailee_l2_verify`)

**Good first issues**
- Add golden-vector tests for snapshot hashing
- Document state canonicalization rules

---

### 2. Bitcoin Anchoring & Bridge Logic

- Review anchor commitment construction (OP_RETURN / Taproot fragments)
- Improve peg-in / peg-out lifecycle enforcement
- Strengthen anchor-bound exit checks
- Improve error handling and operator diagnostics

**Good first issues**
- Add anchor-verification failure test cases
- Improve anchor payload documentation

---

### 3. Security & Cryptography

- Review cryptographic assumptions and threat models
- Audit deterministic hashing and proof wiring
- Improve misuse resistance and failure modes
- Identify edge cases that could compromise verification

> ⚠️ **Do not introduce cryptographic primitives without justification and review.**

**Good first issues**
- Document cryptographic boundaries and non-goals
- Add explicit threat-model notes to verification paths

---

### 4. Systems Engineering (C++)

- Improve thread safety and lifecycle management
- Harden structured logging and diagnostics
- Improve error propagation and shutdown semantics
- Reduce nondeterminism in runtime behavior

**Good first issues**
- Improve structured logging severity levels
- Add deterministic ordering where implicit iteration exists

---

### 5. Documentation

Documentation is considered **production-critical**.

- Clarify security model and trust assumptions
- Improve operator-facing documentation
- Add diagrams that reflect implemented (not aspirational) architecture
- Improve build and verification instructions

**Good first issues**
- Add “How to Verify a Snapshot” walkthrough
- Improve `VERIFICATION.md` clarity

---

## Questions Are Contributions

If something is unclear, that is a documentation bug.

Examples of valid issues:
- “I don’t understand how peg-out authorization is enforced.”
- “What breaks if serialization order changes here?”
- “What security property does this anchor guarantee — and what doesn’t it?”

Clarity is a feature.

---

## Development Workflow

### Prerequisites

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- OpenSSL development libraries

### Build

```bash
git clone https://github.com/dfeen87/AILEE-Core-For-Bitcoin.git
cd AILEE-Core
mkdir build && cd build
cmake ..
make
````

### Tests

Some test coverage exists; additional tests are welcome.

```bash
ctest
```

---

## Pull Request Guidelines

1. Fork the repository
2. Create a feature branch:

   ```bash
   git checkout -b feature/descriptive-name
   ```
3. Commit with **clear, technical messages**:

   * ❌ `Fixed stuff`
   * ✅ `Bind peg-out authorization to anchorCommitmentHash`
4. Include tests where behavior or determinism is affected
5. Update documentation if behavior or assumptions change
6. Open a PR against `main` and explain:

   * What changed
   * Why it matters
   * Any security or compatibility impact

PRs that weaken determinism, blur trust assumptions, or overclaim security will be rejected.

---

## Reporting Issues

Please tag issues appropriately:

* `[BUG]` – crashes, incorrect behavior
* `[SECURITY]` – vulnerabilities or misuse risks
* `[VERIFICATION]` – determinism, state roots, anchors
* `[DOCS]` – missing or unclear documentation
* `[ENHANCEMENT]` – scoped, justified improvements
* `[QUESTION]` – clarification requests
* `[DISCUSSION]` – design or architectural topics

Security issues should follow the instructions in `SECURITY.md`.

---

## Forks & Downstream Use

Forks are welcome.

However, operating a fork of AILEE-Core implies **real responsibility**, including:

* Custodial risk (federated exits)
* Correctness of state and verification
* Honest disclosure of trust assumptions

If you operate a fork, please do not misrepresent the security model.

---

## License

By contributing, you agree that your contributions are licensed under the **MIT License**.

You must have the right to contribute the code you submit.

---

## Final Note

AILEE-Core is not a demo.

It is infrastructure that enforces correctness, exposes failure, and demands clarity.
Contributions should strengthen those properties — not dilute them.

If that excites you, welcome.
