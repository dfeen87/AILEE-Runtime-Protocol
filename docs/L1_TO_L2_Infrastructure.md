# AILEE Layer-1 to Layer-2 Infrastructure

**Complete Technical Guide to Bitcoin Mainnet ↔ AILEE Sidechain Integration**

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Layers](#architecture-layers)
3. [Two-Way Peg Mechanism](#two-way-peg-mechanism)
4. [Security Model](#security-model)
5. [Transaction Flow](#transaction-flow)
6. [Federation Protocol](#federation-protocol)
7. [SPV Proof Verification](#spv-proof-verification)
8. [Emergency Procedures](#emergency-procedures)
9. [Performance Characteristics](#performance-characteristics)
10. [Integration Guide](#integration-guide)

---

## Overview

AILEE implements a two-way peg mechanism between Bitcoin Layer-1 (mainnet) and
AILEE Layer-2 (sidechain), enabling:

- Asset transfers between layers through federated multi-sig
- Off-chain execution with deterministic commitments
- No consensus changes to the Bitcoin protocol
- SPV-based verification for peg-in proofs
- Recovery workflows that can reference Bitcoin-anchored commitments

### Why Layer-2?

Bitcoin's base layer prioritizes **security and decentralization** over execution speed.
AILEE Layer-2 provides off-chain execution and deterministic commitments while preserving
Bitcoin's consensus rules.

```
┌──────────────────────────────────────────────────────────┐
│  Bitcoin L1: Maximum Security & Decentralization         │
│  • Proof of Work consensus                               │
│  • Global validator network                              │
│  • Slow but unstoppable (7 TPS)                         │
└───────────────────┬──────────────────────────────────────┘
                    │
                    │ Two-Way Peg Bridge
                    │
┌───────────────────▼──────────────────────────────────────┐
│  AILEE L2: Off-Chain Execution & Commitments             │
│  • Deterministic L2 state commitments                    │
│  • Orchestration and telemetry execution                 │
│  • Recovery hooks tied to Bitcoin anchoring              │
│  • L2 execution without L1 consensus changes             │
└──────────────────────────────────────────────────────────┘
```

**Key Principle:** Layer-1 provides **final settlement**; Layer-2 provides off-chain execution
with deterministic commitments.

### Canonical L2 State Boundary

The canonical L2 state boundary is limited to:

- **Ledger state:** balances, peg-in/out accounting, and validator-facing transitions.
- **Orchestration state:** task scheduling assignments, reputation/latency tracking, and
  orchestration metrics produced by the scheduling engine.
- **Telemetry commitments:** hashes or proofs derived from telemetry samples and ZK proof outputs.

### L1 Anchoring Boundary

Bitcoin anchoring is limited to deterministic commitments that can be verified externally:

- **Bitcoin adapter commitments:** anchor hashes derived from
  `(L2 state root || timestamp || recovery metadata)` with no broadcast side effects.
- **Recovery hooks:** recovery claims may reference anchor hashes for auditability.

---

## Architecture Layers

### Complete Stack Visualization

```
┌─────────────────────────────────────────────────────────────────┐
│                        USER LAYER                               │
│  Wallets • Exchanges • dApps • Mining Pools                    │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                            │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐   │
│  │  Recovery    │  │  Gold Bridge │  │  Atomic Swaps      │   │
│  │  Protocol    │  │  (BTC↔Gold)  │  │  (P2P Exchange)    │   │
│  └──────────────┘  └──────────────┘  └────────────────────┘   │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                  GOVERNANCE LAYER                               │
│  DAO Voting • Treasury • Validator Registry • Parameter Tuning │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   OPTIMIZATION LAYER                            │
│  AI TPS Engine • Circuit Breaker • Energy Telemetry           │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    CONSENSUS LAYER (L2)                         │
│  Validator Network • Block Production • State Management       │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼ Checkpointing
┌─────────────────────────────────────────────────────────────────┐
│              ★★★ BRIDGE LAYER (Critical) ★★★                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  TWO-WAY PEG MECHANISM                                   │  │
│  │  • Peg-In:  Lock BTC → Mint AILEE                        │  │
│  │  • Peg-Out: Burn AILEE → Release BTC                     │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  FEDERATION (15 Signers, 10/15 Threshold)               │  │
│  │  • Multi-signature security                              │  │
│  │  • Byzantine fault tolerance                             │  │
│  │  • Reputation-based slashing                             │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  SPV PROOF VERIFICATION                                  │  │
│  │  • Merkle proof validation                               │  │
│  │  • Block header verification                             │  │
│  │  • No full Bitcoin node required                         │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                BITCOIN MAINNET (L1) - Final Settlement          │
│  • Proof of Work Consensus                                      │
│  • Global Security ($1T+ network value)                        │
│  • Immutable Ledger                                             │
└─────────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Primary Function | Trust Model | Speed |
|-------|-----------------|-------------|-------|
| **Bitcoin L1** | Final settlement, immutability | Trustless (PoW) | ~10 min/block |
| **Bridge** | Asset transfer, peg security | Federated multi-sig | ~1 hour (6 conf) |
| **AILEE L2** | Off-chain execution, deterministic commitments | Validator consensus | L2-defined |
| **Applications** | User features, recovery, swaps | Smart contract logic | L2-defined |

---

## Two-Way Peg Mechanism

### Peg-In Process (Bitcoin → AILEE)

**Goal:** Lock BTC on mainnet, mint equivalent on AILEE

```
┌──────────────────────────────────────────────────────────────┐
│                    PEG-IN FLOW DIAGRAM                       │
└──────────────────────────────────────────────────────────────┘

Step 1: User Initiates Transfer
    │
    ├─→ User sends BTC to Federation multi-sig address
    │   Example: bc1q...federation_pubkey
    │   Amount: 1.5 BTC
    │
    ▼
Step 2: Bitcoin Transaction Confirmation
    │
    ├─→ Wait for 6 Bitcoin confirmations (~1 hour)
    │   Block 800,000 → Block 800,006
    │
    ▼
Step 3: SPV Proof Generation
    │
    ├─→ Bridge node generates Merkle proof
    │   • Transaction hash
    │   • Merkle branch
    │   • Block header
    │
    ▼
Step 4: Proof Submission to AILEE
    │
    ├─→ User (or relayer) submits SPV proof to AILEE
    │   Contract: SidechainBridge.submitSPVProof()
    │
    ▼
Step 5: Proof Verification
    │
    ├─→ AILEE validators verify:
    │   ✓ Transaction exists in Bitcoin block
    │   ✓ Block is part of longest chain
    │   ✓ Sufficient confirmations (6+)
    │
    ▼
Step 6: Mint AILEE Tokens
    │
    ├─→ Smart contract mints tokens
    │   Amount: 1.5 BTC - 0.00001 BTC (fee) = 1.49999 AILEE
    │   Recipient: User's AILEE address
    │
    ▼
Step 7: Confirmation
    │
    └─→ User receives AILEE tokens
        Transaction complete in ~1 hour
```

### Peg-Out Process (AILEE → Bitcoin)

**Goal:** Burn AILEE tokens, release equivalent BTC

```
┌──────────────────────────────────────────────────────────────┐
│                   PEG-OUT FLOW DIAGRAM                       │
└──────────────────────────────────────────────────────────────┘

Step 1: User Initiates Withdrawal
    │
    ├─→ User burns AILEE tokens on L2
    │   Contract: SidechainBridge.initiatePegOut()
    │   Amount: 0.5 AILEE
    │   Destination: User's Bitcoin address
    │
    ▼
Step 2: AILEE Confirmation Period
    │
    ├─→ Wait for 100 AILEE confirmations (~100 seconds)
    │   Block 50,000 → Block 50,100
    │   Ensures finality on L2
    │
    ▼
Step 3: Federation Signature Collection
    │
    ├─→ Peg-out request broadcast to federation
    │   15 federation members notified
    │
    ├─→ Each member independently verifies:
    │   ✓ Burn transaction is valid
    │   ✓ User has sufficient balance
    │   ✓ No double-spend attempts
    │
    ├─→ Members sign Bitcoin release transaction
    │   Signature 1/15: Fed_Member_1 signs
    │   Signature 2/15: Fed_Member_2 signs
    │   ...
    │   Signature 10/15: Fed_Member_10 signs ✓ THRESHOLD MET
    │
    ▼
Step 4: Bitcoin Transaction Broadcast
    │
    ├─→ Once 10/15 signatures collected:
    │   • Construct Bitcoin transaction
    │   • Sign with multi-sig (10 of 15)
    │   • Broadcast to Bitcoin network
    │
    ▼
Step 5: Bitcoin Confirmation
    │
    ├─→ Wait for Bitcoin confirmation
    │   Block 800,100 mined
    │
    ▼
Step 6: Release Complete
    │
    └─→ User receives BTC on mainnet
        Amount: 0.5 BTC - 0.00001 BTC (fee) = 0.49999 BTC
        Transaction complete in ~2-3 hours
```

### Critical Parameters

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Peg-In Confirmations** | 6 Bitcoin blocks | Standard industry practice, ~1 hour |
| **Peg-Out Confirmations** | 100 AILEE blocks | Ensures L2 finality, ~100 seconds |
| **Min Peg Amount** | 0.0001 BTC (10,000 sats) | Prevents spam, covers fees |
| **Max Peg Amount** | 100 BTC | Risk management per transaction |
| **Bridge Fee** | 0.00001 BTC (1,000 sats) | Covers operational costs |
| **Federation Threshold** | 10 of 15 signatures | 2/3 supermajority, BFT |

---

## Security Model

### Multi-Layered Defense

```
┌─────────────────────────────────────────────────────────────┐
│                   SECURITY LAYERS                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Layer 1: Cryptographic Proofs                             │
│  ┌───────────────────────────────────────────────────┐     │
│  │ • SPV Merkle proofs (trustless verification)     │     │
│  │ • Multi-signature schemes (10 of 15)             │     │
│  │ • Hash time-locks (atomic swaps)                 │     │
│  └───────────────────────────────────────────────────┘     │
│                                                             │
│  Layer 2: Economic Security                                │
│  ┌───────────────────────────────────────────────────┐     │
│  │ • Federation members stake ADU tokens            │     │
│  │ • Slashing for malicious behavior                │     │
│  │ • Reputation scoring system                      │     │
│  └───────────────────────────────────────────────────┘     │
│                                                             │
│  Layer 3: Operational Security                             │
│  ┌───────────────────────────────────────────────────┐     │
│  │ • Geographic distribution of federation          │     │
│  │ • Redundant monitoring systems                   │     │
│  │ • Real-time anomaly detection                    │     │
│  └───────────────────────────────────────────────────┘     │
│                                                             │
│  Layer 4: Emergency Procedures                             │
│  ┌───────────────────────────────────────────────────┐     │
│  │ • Circuit breaker auto-halt                      │     │
│  │ • Emergency mode (pause pegs)                    │     │
│  │ • Time-locked recovery (1 week timeout)          │     │
│  └───────────────────────────────────────────────────┘     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Threat Model & Mitigations

| Threat | Attack Vector | Mitigation | Success Probability |
|--------|--------------|------------|---------------------|
| **Double-Spend** | Reorg Bitcoin chain | 6-confirmation requirement | <0.01% |
| **Federation Collusion** | 10+ members collude | Stake slashing + reputation | <0.1% (economic incentive) |
| **SPV Proof Forgery** | Fake Merkle proof | Cryptographic verification | 0% (computationally infeasible) |
| **Peg-Out Theft** | Steal locked BTC | Multi-sig + time-locks | 0% (requires 10/15 keys) |
| **Under-collateralization** | More AILEE than BTC locked | Real-time monitoring + circuit breaker | Auto-halts at <90% |
| **DoS Attack** | Spam peg requests | Rate limiting + minimum amounts | Mitigated by fees |
| **Long-Range Attack** | Rewrite L2 history | Bitcoin checkpointing every 100 blocks | 0% (Bitcoin PoW security) |

### Byzantine Fault Tolerance

The federation can tolerate up to **5 malicious members** (1/3 of 15):

```
Honest Members Needed: 10/15 (66.67%)
Maximum Malicious: 5/15 (33.33%)

Scenario Analysis:
  15 total signers
  - 10 honest (required for operation)
  - 5 malicious (cannot execute peg-out alone)

Malicious members CANNOT:
  ✗ Release BTC without honest majority
  ✗ Block peg-outs indefinitely (emergency timeout)
  ✗ Forge SPV proofs (cryptographically impossible)
  ✗ Double-spend AILEE (blockchain consensus prevents)

Malicious members CAN (limited impact):
  ✓ Refuse to sign (but 10 others can complete)
  ✓ Delay peg-outs slightly (but reputation slashed)
  ✓ Exit federation (but network continues)
```

### Collateralization Guarantee

```
Collateralization Ratio = (Locked BTC on L1) / (Minted AILEE on L2)

Target Ratio: 1.00 (1:1 backing)
Warning Zone: < 0.95 (95% collateralized)
Emergency Zone: < 0.90 (90% collateralized)

At <90% collateralization:
  1. Circuit breaker activates
  2. New peg-ins HALTED
  3. Existing peg-outs processed normally
  4. DAO governance notified
  5. Emergency audit initiated

Recovery Process:
  • Identify discrepancy source
  • Federation votes on resolution
  • Additional BTC locked if needed
  • Normal operations resume at >95%
```

---

## Transaction Flow

### Complete Peg-In Sequence Diagram

```
User          Bitcoin       Bridge         AILEE        Federation
 │              │             │              │              │
 │─────────────▶│             │              │              │
 │ Send 1 BTC   │             │              │              │
 │ to multi-sig │             │              │              │
 │              │             │              │              │
 │              │─ Conf 1 ───▶│              │              │
 │              │─ Conf 2 ───▶│              │              │
 │              │─ Conf 3 ───▶│              │              │
 │              │─ Conf 4 ───▶│              │              │
 │              │─ Conf 5 ───▶│              │              │
 │              │─ Conf 6 ───▶│ ✓            │              │
 │              │             │              │              │
 │──────────────────────────▶│              │              │
 │ Submit SPV proof           │              │              │
 │                            │              │              │
 │                            │─ Verify ────▶│              │
 │                            │              │              │
 │                            │              │─ Validate ──▶│
 │                            │              │              │
 │                            │              │◀─ Approved ──│
 │                            │              │              │
 │                            │◀─ Mint 1 AILEE─│            │
 │                            │              │              │
 │◀───────────────────────────────────────────────────────────│
 │ Receive 1 AILEE (minus fee)               │              │
 │                                           │              │
```

### Complete Peg-Out Sequence Diagram

```
User         AILEE        Federation      Bitcoin
 │             │              │              │
 │────────────▶│              │              │
 │ Burn 0.5    │              │              │
 │ AILEE       │              │              │
 │             │              │              │
 │             │─ Conf 1-100 ─▶              │
 │             │              │              │
 │             │──────────────▶│              │
 │             │ Request sigs  │              │
 │             │              │              │
 │             │              │─ Member 1    │
 │             │              │   signs      │
 │             │              │─ Member 2    │
 │             │              │   signs      │
 │             │              │─ ...         │
 │             │              │─ Member 10   │
 │             │              │   signs ✓    │
 │             │              │              │
 │             │              │──────────────▶│
 │             │              │ Broadcast TX  │
 │             │              │              │
 │             │              │              │─ Mined
 │             │              │              │
 │◀──────────────────────────────────────────────────────│
 │ Receive 0.5 BTC (minus fee)               │          │
 │                                           │          │
```

---

## Federation Protocol

### Member Selection & Requirements

**Eligibility Criteria:**
- Minimum 100,000 ADU stake (locked for participation)
- Technical infrastructure (24/7 uptime, secure HSM)
- Geographic diversity (max 2 members per region)
- Reputation score >80/100
- DAO governance approval vote

**Geographic Distribution:**
```
Region           Members    % of Federation
────────────────────────────────────────────
North America    3          20%
Europe           3          20%
Asia-Pacific     4          27%
Latin America    2          13%
Middle East      2          13%
Africa           1          7%
────────────────────────────────────────────
TOTAL           15         100%
```

### Signature Collection Process

```
┌─────────────────────────────────────────────────────────┐
│          FEDERATED SIGNATURE WORKFLOW                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Peg-Out Request Broadcast                          │
│     ├─→ All 15 members notified simultaneously         │
│     └─→ Request includes: amount, destination, proof   │
│                                                         │
│  2. Independent Verification (Each Member)             │
│     ├─→ Check burn transaction validity                │
│     ├─→ Verify no double-spend                         │
│     ├─→ Confirm collateralization ratio                │
│     └─→ Validate destination address                   │
│                                                         │
│  3. Signing Decision                                   │
│     ├─→ If valid: Sign with private key                │
│     ├─→ If invalid: Refuse + report reason             │
│     └─→ Timeout: 30 minutes to respond                 │
│                                                         │
│  4. Signature Aggregation                              │
│     ├─→ Coordinator collects signatures                │
│     ├─→ Validates each signature                       │
│     └─→ Threshold check: 10/15 required                │
│                                                         │
│  5. Transaction Construction                           │
│     ├─→ Build Bitcoin transaction                      │
│     ├─→ Attach multi-sig (10 signatures)               │
│     └─→ Broadcast to Bitcoin network                   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Reputation & Slashing

**Reputation Scoring (0-100):**
```
Starting Score: 100

Positive Actions:
  + Sign peg-out successfully: +0 (maintain)
  + Uptime >99.9% monthly: +1
  + Early response (<5 min): +1

Negative Actions:
  - Miss signature request: -5
  - Invalid signature: -10
  - Downtime >1 hour: -3
  - False fraud report: -15

Automatic Penalties:
  • Score <80: Warning issued
  • Score <60: Temporary suspension
  • Score <40: Removal from federation
  • 10 missed signatures: Immediate removal
```

**Slashing Conditions:**
```
Offense                    Penalty
───────────────────────────────────────────
Malicious signature        100% stake slash
Attempted double-spend     100% stake + ban
Colluding with others      100% stake + ban
Extended downtime (>24h)   10% stake slash
Repeated missed sigs       5% stake per miss
False emergency claims     20% stake slash
```

---

## SPV Proof Verification

### Simplified Payment Verification Explained

SPV allows verification of Bitcoin transactions **without downloading the entire blockchain** (500+ GB). Instead, only block headers (80 bytes each) are needed.

**How It Works:**

```
┌──────────────────────────────────────────────────────────┐
│               BITCOIN BLOCK STRUCTURE                    │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  Block Header (80 bytes) ─── SPV only needs this!       │
│  ├─ Version (4 bytes)                                   │
│  ├─ Previous Block Hash (32 bytes)                      │
│  ├─ Merkle Root (32 bytes) ◄──── Critical for SPV      │
│  ├─ Timestamp (4 bytes)                                 │
│  ├─ Difficulty Target (4 bytes)                         │
│  └─ Nonce (4 bytes)                                     │
│                                                          │
│  Transactions (variable size) ─── SPV does NOT need     │
│  ├─ Transaction 1                                       │
│  ├─ Transaction 2                                       │
│  ├─ Transaction 3  ◄──── Our peg-in transaction        │
│  ├─ ...                                                 │
│  └─ Transaction N                                       │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### Merkle Proof Construction

```
                    Merkle Root
                   (in block header)
                         │
             ┌───────────┴───────────┐
             │                       │
          Hash AB                 Hash CD
             │                       │
       ┌─────┴─────┐          ┌────┴─────┐
       │           │          │          │
     Hash A     Hash B     Hash C     Hash D
       │           │          │          │
     TX A        TX B       TX C       TX D
                            ▲
                            │
                    Our Peg-In Transaction

SPV Proof Contains:
  1. TX C (our transaction)
  2. Hash D (sibling)
  3. Hash AB (sibling)
  
Verification:
  Hash C = SHA256(SHA256(TX C))
  Hash CD = SHA256(Hash C + Hash D)
  Merkle Root = SHA256(Hash AB + Hash CD)
  
  ✓ If calculated Merkle Root matches block header → Valid!
```

### Implementation

```cpp
// Verify SPV proof
bool SPVProof::verify(
    const ProofData& proof,
    const std::vector<uint8_t>& blockHeader
) {
    // 1. Extract merkle root from block header (bytes 36-68)
    std::vector<uint8_t> merkleRoot(
        blockHeader.begin() + 36,
        blockHeader.begin() + 68
    );
    
    // 2. Calculate transaction hash
    std::vector<uint8_t> currentHash = doubleSHA256(proof.transaction);
    
    // 3. Traverse merkle tree using proof
    for (const auto& sibling : proof.merkleProof) {
        // Concatenate in correct order
        std::vector<uint8_t> combined;
        if (currentHash < sibling) {
            combined = currentHash + sibling;
        } else {
            combined = sibling + currentHash;
        }
        
        // Hash again
        currentHash = doubleSHA256(combined);
    }
    
    // 4. Compare with merkle root
    return currentHash == merkleRoot;
}
```

### Security Guarantees

**What SPV Proves:**
✅ Transaction exists in a specific Bitcoin block  
✅ Block was mined (proof of work)  
✅ Transaction is part of longest chain (most work)  

**What SPV Does NOT Prove:**
❌ Transaction is valid (inputs exist and are unspent)  
❌ No double-spend in same block  
❌ Script execution is correct  

**Solution:** Require 6 confirmations. If a transaction were invalid, the Bitcoin network would reject the block, and it wouldn't achieve 6 confirmations on the honest chain.

---

## Emergency Procedures

### Circuit Breaker Activation

**Automatic Triggers:**
```
Condition                          Action
────────────────────────────────────────────────────
Collateralization < 90%           → Halt new peg-ins
Federation < 10 active signers    → Emergency mode
Abnormal peg volume (>1000 BTC/h) → Rate limiting
SPV proof failures > 5%           → Manual review
Bitcoin network fork detected     → Pause all pegs
```

### Emergency Mode Operations

```
┌─────────────────────────────────────────────────────────┐
│            EMERGENCY MODE PROCEDURES                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Phase 1: Immediate Response (0-1 hour)                │
│  ├─→ Halt all new peg-ins                              │
│  ├─→ Continue processing existing peg-outs             │
│  ├─→ Notify DAO governance                             │
│  └─→ Alert all federation members                      │
│                                                         │
│  Phase 2: Investigation (1-24 hours)                   │
│  ├─→ Forensic analysis of trigger                      │
│  ├─→ Audit all recent transactions                     │
│  ├─→ Verify collateralization manually                 │
│  └─→ Identify root cause                               │
│                                                         │
│  Phase 3: Resolution (24-168 hours)                    │
│  ├─→ DAO governance vote on fix                        │
│  ├─→ Implement corrective measures                     │
│  ├─→ Add additional collateral if needed               │
│  └─→ Update bridge parameters                          │
│                                                         │
│  Phase 4: Recovery (After resolution)                  │
│  ├─→ Gradual peg-in reopening                          │
│  ├─→ Enhanced monitoring period                        │
│  ├─→ Post-mortem report published                      │
│  └─→ Community transparency update                     │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Time-Locked Recovery

If federation becomes unresponsive (e.g., >50% members offline):

```
Emergency Timeout: 1008 Bitcoin blocks (~1 week)

After 1 week of no federation activity:
  1. Users can submit direct Bitcoin proofs
  2. Smart contract verifies 1008-block wait
  3. Automatic peg-out executed
  4. No federation signatures required
  
This ensures funds are NEVER permanently locked.
```

---

## Performance Characteristics

### Latency Breakdown

**Peg-In Timeline:**
```
Action                    Time          Cumulative
─────────────────────────────────────────────────────
Submit BTC transaction    1 minute      0:01
Wait for 1st confirmation 10 minutes    0:11
Wait for 6 confirmations  60 minutes    1:11
Generate SPV proof        1 minute      1:12
Submit to AILEE          1 minute      1:13
Verify and mint          10 seconds    1:13:10
───────────────────────────────────────────────────
TOTAL PEG-IN TIME:       ~1 hour 13 minutes
```

**Peg-Out Timeline:**
Peg-Out Timeline:

Action                         Time          Cumulative
────────────────────────────────────────────────────────
Burn AILEE tokens              10 sec        0:00:10
Wait for 100 L2 confirmations 100 sec        0:01:50
Federation signature collection 5-15 min     0:06:50
Construct Bitcoin TX           1 min         0:07:50
Broadcast to Bitcoin network   1-10 min      0:17:50
Wait for 6 Bitcoin confirmations ~1 hour    1:17:50
────────────────────────────────────────────────────────
TOTAL PEG-OUT TIME:           ~1 hr 18 min – 2 hr 30 min

Integration Guide
Prerequisites

Bitcoin full node (v24+)

C++17 compatible compiler

OpenSSL 3.x

Docker (optional, for isolated deployment)

Step 1: Clone & Build
git clone https://github.com/YourRepo/AILEE-Core.git
cd AILEE-Core
mkdir build && cd build
cmake ..
make

Step 2: Configure Nodes

Edit config.json:

{
  "bitcoin_rpc": "http://user:pass@127.0.0.1:8332",
  "ailee_rpc": "http://127.0.0.1:5000",
  "federation_pubkeys": ["pubkey1","pubkey2",...],
  "min_peg": 0.0001,
  "max_peg": 100
}

Step 3: Launch AILEE Sidechain
./ailee_node --config ../config.json

Step 4: Perform Peg-In

Send BTC to the federation multi-sig address.

Generate SPV proof using ailee_spv_tool.

Submit proof via SidechainBridge.submitSPVProof().

Receive minted AILEE tokens in your wallet.

Step 5: Perform Peg-Out

Burn AILEE tokens via SidechainBridge.initiatePegOut().

Wait for L2 confirmations and federation signature aggregation.

Bitcoin is released to your address.

Step 6: Monitoring & Analytics

Use ailee_monitor for real-time TPS, peg status, and collateralization ratio.

Integrates with Grafana/Prometheus dashboards for automated alerts.

Best Practices

Always verify SPV proofs before submission.

Maintain at least 2 independent AILEE nodes per critical application.

Monitor federation reputation regularly.

Implement rate limiting on high-volume peg requests.

Use circuit breaker logs to audit unusual activity.

Notes

Peg-in and peg-out operations are federated with multisig security; exits are not trustless and require signer approval.

Collateralization must remain ≥95% for safe operation; <90% triggers emergency protocols.

Latency on L2 is sub-second, but Bitcoin settlement remains dependent on PoW confirmations.

The system is designed to scale horizontally; additional L2 validators increase throughput without affecting L1 security.

License

MIT License — See LICENSE.md for details.
Author: Don Michael Feeney Jr. — System Architect & Believer
