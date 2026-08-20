import { KernelClockBridge } from '../../src/core/KernelClockBridge.js';
import { CommClockAggregator } from '../../src/core/CommClockAggregator.js';
import { ClockCoherenceEngine } from '../../src/core/ClockCoherenceEngine.js';
import { KernelClockAPI, KernelTime, KernelTimeEvent } from '../../src/ailee-core/types.js';
import { MockRFClockAdapter } from './mockAdapters.js';

class TestKernelClock implements KernelClockAPI {
  private listener?: (event: KernelTimeEvent) => void;

  public getKernelTime(): KernelTime {
    return {
      blockHeight: 100,
      medianBlockTime: 1000n,
      mempoolTimestamp: 5_000_000_000n,
      unixNs: 5_000_000_000n,
    };
  }

  public subscribeKernelClock(callback: (event: KernelTimeEvent) => void): () => void {
    this.listener = callback;
    return () => {
      this.listener = undefined;
    };
  }

  public triggerEvent(event: KernelTimeEvent): void {
    if (this.listener) {
      this.listener(event);
    }
  }
}

describe('KernelClockBridge', () => {
  test('correlates comm events and emits bridge event on kernel clock update', () => {
    const kernelClock = new TestKernelClock();
    const aggregator = new CommClockAggregator();
    const coherenceEngine = new ClockCoherenceEngine();
    const rfAdapter = new MockRFClockAdapter();

    aggregator.registerRFDomain(rfAdapter);

    const bridge = new KernelClockBridge(kernelClock, aggregator, coherenceEngine);

    const receivedEvents: any[] = [];
    const unsub = bridge.subscribeKernelCommBridge((e) => receivedEvents.push(e));

    // Fire RF event
    rfAdapter.fireSymbolEvent(5_000_100_000n, 10n);

    // Fire Kernel Clock event
    kernelClock.triggerEvent({
      kernelTime: kernelClock.getKernelTime(),
      type: 'KERNEL_TICK',
    });

    expect(receivedEvents.length).toBe(1);
    const bridgeEvent = receivedEvents[0];
    expect(bridgeEvent.kernelTime.blockHeight).toBe(100);
    expect(bridgeEvent.correlatedCommEvents.length).toBe(1);
    expect(bridgeEvent.correlatedCommEvents[0].domainId).toBe('RF');
    expect(bridgeEvent.coherenceReports.length).toBe(1);

    unsub();
    bridge.shutdown();
    aggregator.shutdown();
  });
});
