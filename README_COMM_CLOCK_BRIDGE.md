# AILEE-CommClockBridge (ACB)

**AILEE-CommClockBridge (ACB)** is an add-on software module for the **AILEE Runtime**. It extends AILEE's Bitcoin-aligned internal Kernel Clock to synchronize with high-speed, communication-grade hardware clock domains without modifying the underlying AILEE core code.

---

## 1. System Architecture

ACB operates purely via public extension points, observer hooks, and abstract hardware adapters:

```
+-----------------------------------------------------------------------+
|                         AILEE Core Kernel                             |
|       (Bitcoin Mainnet Clock: Block Height, Median Time, Mempool)     |
+-----------------------------------------------------------------------+
                                  |
                        KernelClockAPI Observer
                                  v
+-----------------------------------------------------------------------+
|                    AILEE-CommClockBridge (ACB)                        |
|                                                                       |
|  +---------------------+   +---------------------+   +-------------+  |
|  | Multi-Domain        |   | Clock Coherence     |   | Kernel-Comm |  |
|  | Clock Aggregator    |   | Engine              |   | Bridge      |  |
|  +---------------------+   +---------------------+   +-------------+  |
|             |                         |                     |         |
|             +-------------------------+---------------------+         |
|                                       |                               |
|                            Replay & Introspection API                 |
+-----------------------------------------------------------------------+
                                  ^
                       Abstract Clock Adapters
                                  |
  +------------+------------+------------+------------+------------+
  |            |            |            |            |            |
  v            v            v            v            v            v
[ RF ]   [MixedSignal]    [ DSP ]    [Security]  [Compression] [Network] [Power]
```

---

## 2. Supported Clock Domains

ACB defines standard interfaces for seven primary communication hardware clock domains:

1. **RF / Radio Frequency Clock Domain (`RFClockDomain`)**
   - Transceivers, phase-coherent synthesizers, beam-forming timing.
2. **High-Performance Analog / Mixed-Signal Clock Domain (`MixedSignalClockDomain`)**
   - ADC/DAC sampling pipelines, conversion latency tracking.
3. **DSP Clock Domain (`DSPClockDomain`)**
   - Baseband processing (5G NR, LTE frame/subframe timing, OFDM).
4. **Security Engine Clock Domain (`SecurityClockDomain`)**
   - Crypto accelerators and hardware secure enclaves.
5. **Data Compression Clock Domain (`CompressionClockDomain`)**
   - Hardware compression blocks for backhaul and optical transceivers.
6. **Networking Layer Clock Domain (`NetworkClockDomain`)**
   - Switches, Wi-Fi 6E/7, DOCSIS cable modems, PON, DSL modems, PTP timestamps.
7. **Power Management Clock Domain (`PowerClockDomain`)**
   - PMICs, dynamic voltage-frequency scaling (DVFS), clock gating states.

---

## 3. Core Components

### 3.1 Multi-Domain Clock Aggregator (`CommClockAggregator`)
Collects and indexes events across all registered clock domains into a `UnifiedCommTimeline`.
- **`UnifiedCommTimeline`**: High-resolution buffer providing windowed queries (`getEventsBetween`) and time-mapping to Bitcoin kernel snapshots (`mapToKernelTime`).

### 3.2 Clock Coherence Engine (`ClockCoherenceEngine`)
Computes real-time coherence metrics between Bitcoin mainnet time and hardware clock domains:
- **`driftNsPerSecond`**: Frequency drift rate relative to Bitcoin kernel ticks.
- **`jitterNs`**: Standard deviation of timing delta over rolling window.
- **`alignmentConfidence`**: Normalized score (0.0 to 1.0) quantifying temporal alignment.

### 3.3 Kernel Clock Bridge (`KernelClockBridge`)
Subscribes to AILEE `KernelClockAPI`, correlates hardware communication events with incoming Bitcoin blocks/mempool events, and emits unified `KernelCommBridgeEvent` objects.

### 3.4 Replay & Introspection API (`CommReplayAPI`)
Allows engineers to replay historical communication hardware events aligned against Bitcoin mainnet block windows with configurable domain filters and confidence thresholds.

---

## 4. Configuration & Vendor Profiles

Vendor configuration profiles allow ACB to be deployed across different telecom hardware architectures without code changes. Profiles are stored in `configs/`:

- `configs/maxlinear_rf_dsp_network.json` (MaxLinear 5G NR / Baseband profile)
- `configs/generic_optical_transceiver.json` (High-speed optical transport profile)
- `configs/home_wifi_router.json` (Wi-Fi 6E / DOCSIS gateway profile)

---

## 5. Vendor Integration Guide (e.g. MaxLinear)

Follow these steps to integrate MaxLinear hardware with AILEE-CommClockBridge:

### Step 1: Clone Repository & Install Dependencies
```bash
npm install
npm run build
```

### Step 2: Implement Vendor Clock Adapters
Extend the abstract adapter classes in `src/domains/adapters/` (or use the provided stubs in `src/domains/vendor/maxlinear/`):

```typescript
import { RFClockAdapter } from 'ailee-commclock-bridge';

export class MaxLinearRFClockAdapter extends RFClockAdapter {
  constructor(private hardwareHandle: any) {
    super();
  }

  public getCurrentPhase(): number {
    return this.hardwareHandle.readRegister(0x40001000);
  }

  public getCarrierFrequencyHz(): number {
    return 3_500_000_000;
  }

  public getSymbolRateHz(): number {
    return 30_720_000;
  }

  public getTimestamp(): CommClockTimestamp {
    return {
      unixNs: BigInt(this.hardwareHandle.getPtpTimerNs()),
      domainId: 'RF',
      sourceId: 'MAXLINEAR_RF_0',
    };
  }

  public subscribeRFClock(callback: (event: RFClockEvent) => void): () => void {
    return this.hardwareHandle.onSymbolInterrupt(callback);
  }
}
```

### Step 3: Create a Vendor Profile Configuration
Create `configs/my_hardware_profile.json`:
```json
{
  "profileName": "my_hardware_profile",
  "vendorName": "MaxLinear",
  "domains": [
    {
      "domainId": "RF",
      "enabled": true,
      "adapterClassName": "MaxLinearRFClockAdapter",
      "parameters": { "sourceId": "MXL_RF_0" }
    }
  ]
}
```

### Step 4: Run Tests & Validate Coherence
```bash
npm test
```

---

## 6. Running Examples

To execute the end-to-end example demonstration:
```bash
npm run build
node dist/examples/usage.js
```
