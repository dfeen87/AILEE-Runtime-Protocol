# **AILEE CORE — V12 Reproducibility Package**  
**Deterministic Build • Deterministic Execution • Deterministic Replay**

This document defines the full reproducibility package for **AILEE CORE V12**, enabling any engineer, auditor, or automated system to reproduce the build, verify the execution outputs, and deterministically replay the protocol from genesis through epoch 4.

It contains four canonical artifacts:

1. **Frozen Genesis**  
2. **Build Manifest + Build Hash**  
3. **Receipt Manifest + Receipt Hash**  
4. **Re‑Execution Path + Re‑Execution Hash**

These artifacts guarantee that V12 can be reproduced bit‑for‑bit and state‑for‑state on any compliant execution environment.

---

## **1. Frozen Genesis**

**File:** `v12_frozen_genesis.json`  
**Purpose:** Defines the immutable starting state for V12.  
**Canonical Genesis State Root:**

```
425e70c240a19594bac19c354a12fa7edbe6e6c6d3c5790e32b5af52de6ff8ee
```

This value anchors all subsequent state transitions and epoch roots.

---

## **2. Build Manifest & Build Hash**

### **Build Manifest**  
**File:** `v12_build_manifest.json`  
Contains:

- Binary hash  
- Container hash  
- Deterministic environment  
- Execution command  
- Version tag  

### **Build Hash**  
**File:** `v12_build_hash.txt`  
**Canonical Build Hash:**

```
e9576ab0527e7e3fb27230936be817f3cf52fd73f4955a91eae5c282c04bd75a
```

This hash is computed over the entire build manifest and represents the fingerprint of the V12 build environment.

---

## **3. Receipt Manifest & Receipt Hash**

### **Receipt Manifest**  
**File:** `v12_receipt_manifest.json`  
Contains:

- Genesis state root  
- Epoch state roots (0–4)  
- Deterministic execution environment  
- Execution command  

### **Receipt Hash**  
**File:** `v12_receipt_hash.txt`  
**Canonical Receipt Hash:**

```
b02a21d807ce351b641bcacadc2f32243e0135996cd8fd34d96a9f71fe8dfdfd
```

This hash fingerprints the deterministic execution outputs of V12.

---

## **4. Re‑Execution Path & Re‑Execution Hash**

### **Re‑Execution Path Manifest**  
**File:** `v12_reexecution_manifest.json`  
Contains:

- Build hash  
- Receipt hash  
- Genesis root  
- Required files  
- Deterministic environment  
- Expected epoch state roots  
- Step‑by‑step replay instructions  

### **Re‑Execution Hash**  
**File:** `v12_reexecution_hash.txt`  
Computed via:

```
shasum -a 256 v12_reexecution_manifest.json
```

This hash fingerprints the replay instructions themselves.

---

## **Deterministic Replay Instructions**

To replay V12 deterministically:

1. Verify the build hash matches `v12_build_manifest.json`.  
2. Verify the receipt hash matches `v12_receipt_manifest.json`.  
3. Load `v12_frozen_genesis.json` as the initial state.  
4. Execute:

   ```
   ./ailee_core_v12.bin --run-epoch 0
   ```

5. Capture the state root for epoch 0 and verify it matches the expected value.  
6. Continue execution through epochs 1–4.  
7. Confirm all epoch roots match the expected table in the reexecution manifest.  
8. If all roots match, the replay is successful and deterministic.

---

## **Artifact Summary**

| Artifact | File | Purpose |
|---------|------|---------|
| **Frozen Genesis** | `v12_frozen_genesis.json` | Immutable starting state |
| **Build Manifest** | `v12_build_manifest.json` | Build environment definition |
| **Build Hash** | `v12_build_hash.txt` | Fingerprint of build environment |
| **Receipt Manifest** | `v12_receipt_manifest.json` | Execution outputs |
| **Receipt Hash** | `v12_receipt_hash.txt` | Fingerprint of execution outputs |
| **Re‑Execution Path** | `v12_reexecution_manifest.json` | Deterministic replay instructions |
| **Re‑Execution Hash** | `v12_reexecution_hash.txt` | Fingerprint of replay instructions |
