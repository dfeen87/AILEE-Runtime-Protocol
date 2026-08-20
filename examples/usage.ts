import {
  KernelClockAPI,
  KernelTime,
  KernelTimeEvent,
  CommClockAggregator,
  ClockCoherenceEngine,
  KernelClockBridge,
  CommReplayAPI,
  CommClockConfig,
  MaxLinearRFClockAdapter,
  MaxLinearDSPClockAdapter,
  MaxLinearNetworkClockAdapter,
} from '../src/index.js';

/**
 * Mock AILEE Kernel Clock API implementing Bitcoin-aligned timing.
 */
class MockKernelClock implements KernelClockAPI {
  private currentBlockHeight = 880_000;
  private nowNs = BigInt(Date.now()) * 1_000_000n;
  private readonly listeners: Set<(event: KernelTimeEvent) => void> = new Set();

  public getKernelTime(): KernelTime {
    return {
      blockHeight: this.currentBlockHeight,
      medianBlockTime: BigInt(Math.floor(Date.now() / 1000) - 3600),
      mempoolTimestamp: this.nowNs,
      unixNs: this.nowNs,
    };
  }

  public subscribeKernelClock(callback: (event: KernelTimeEvent) => void): () => void {
    this.listeners.add(callback);
    return () => {
      this.listeners.delete(callback);
    };
  }

  public emitTick(): void {
    this.nowNs += 1_000_000_000n; // 1 second tick
    const event: KernelTimeEvent = {
      kernelTime: this.getKernelTime(),
      type: 'KERNEL_TICK',
    };
    for (const cb of this.listeners) {
      cb(event);
    }
  }

  public emitBlock(hash: string): void {
    this.currentBlockHeight += 1;
    this.nowNs += 1_000_000_000n;
    const event: KernelTimeEvent = {
      kernelTime: this.getKernelTime(),
      type: 'BLOCK_CONNECTED',
      payloadHash: hash,
    };
    for (const cb of this.listeners) {
      cb(event);
    }
  }
}

async function main() {
  console.log('=== Initializing AILEE-CommClockBridge (ACB) ===');

  // 1. Load Vendor Configuration Profile
  const configPath = 'configs/maxlinear_rf_dsp_network.json';
  const config = CommClockConfig.loadFromFile(configPath);
  console.log(`Loaded Profile: ${config.profileName} (Vendor: ${config.vendorName})`);

  // 2. Initialize AILEE Kernel Clock Mock & Core ACB Subsystems
  const kernelClock = new MockKernelClock();
  const aggregator = new CommClockAggregator();
  const coherenceEngine = new ClockCoherenceEngine();
  const bridge = new KernelClockBridge(kernelClock, aggregator, coherenceEngine);
  const replayApi = new CommReplayAPI(aggregator, coherenceEngine);

  // 3. Instantiate and Register Vendor Hardware Adapters
  const rfAdapter = new MaxLinearRFClockAdapter({ sourceId: 'MXL_RF_0' });
  const dspAdapter = new MaxLinearDSPClockAdapter({ sourceId: 'MXL_DSP_0' });
  const netAdapter = new MaxLinearNetworkClockAdapter({ sourceId: 'MXL_NET_0' });

  aggregator.registerRFDomain(rfAdapter);
  aggregator.registerDSPDomain(dspAdapter);
  aggregator.registerNetworkDomain(netAdapter);

  console.log(`Registered Communication Domains: ${aggregator.getActiveDomainIds().join(', ')}`);

  // 4. Subscribe to Kernel-Comm Bridge Events
  const unsubBridge = bridge.subscribeKernelCommBridge((bridgeEvent) => {
    console.log(
      `\n[Bridge Event] Kernel Time (Block ${bridgeEvent.kernelTime.blockHeight}): ` +
        `${bridgeEvent.correlatedCommEvents.length} correlated comm events`
    );
    for (const report of bridgeEvent.coherenceReports) {
      console.log(
        `  -> Domain '${report.domainId}': Drift = ${report.driftNsPerSecond.toFixed(2)} ns/s, ` +
          `Jitter = ${report.jitterNs.toFixed(2)} ns, Confidence = ${report.alignmentConfidence.toFixed(4)}`
      );
    }
  });

  // 5. Simulate Hardware Events and Kernel Ticks
  const nowNs = BigInt(Date.now()) * 1_000_000n;

  rfAdapter.emitHardwareEvent({
    timestamp: { unixNs: nowNs + 100_000n, domainId: 'RF', sourceId: 'MXL_RF_0' },
    phaseRadians: 1.57,
    carrierFrequencyHz: 3.5e9,
    symbolRateHz: 30.72e6,
    symbolBoundaryIndex: 1001n,
  });

  dspAdapter.emitHardwareEvent({
    timestamp: { unixNs: nowNs + 200_000n, domainId: 'DSP', sourceId: 'MXL_DSP_0' },
    basebandClockHz: 122.88e6,
    frameNumber: 42,
    subframeIndex: 3,
  });

  netAdapter.emitHardwareEvent({
    timestamp: { unixNs: nowNs + 150_000n, domainId: 'NETWORK', sourceId: 'MXL_NET_0' },
    networkClockHz: 156.25e6,
    currentInterfaceId: 'eth0',
    currentPacketSequence: 9982,
  });

  // Trigger Kernel Clock ticks
  kernelClock.emitTick();
  kernelClock.emitBlock('000000000000000000021a8d05e2e811c7df0e3b');

  // 6. Demonstrate Replay API
  const startTime = kernelClock.getKernelTime();
  const endTime: KernelTime = {
    ...startTime,
    unixNs: startTime.unixNs + 10_000_000_000n,
  };

  const replaySession = replayApi.replayCommEventsAlignedToKernel(startTime, endTime, {
    includeDomains: ['RF', 'NETWORK'],
    coherenceThreshold: 0.5,
  });

  console.log(`\n=== Replay Session Results ===`);
  console.log(`Matched Events: ${replaySession.totalEventsMatched}`);
  console.log(`Average Alignment Confidence: ${replaySession.averageConfidence.toFixed(4)}`);

  // Cleanup
  unsubBridge();
  bridge.shutdown();
  aggregator.shutdown();

  console.log('\nAILEE-CommClockBridge initialization and run completed successfully.');
}

if (require.main === module) {
  main().catch(console.error);
}
