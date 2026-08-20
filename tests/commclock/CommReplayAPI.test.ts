import { CommReplayAPI } from '../../src/core/CommReplayAPI.js';
import { CommClockAggregator } from '../../src/core/CommClockAggregator.js';
import { ClockCoherenceEngine } from '../../src/core/ClockCoherenceEngine.js';
import { KernelTime } from '../../src/ailee-core/types.js';
import { MockRFClockAdapter, MockDSPClockAdapter } from './mockAdapters.js';

describe('CommReplayAPI', () => {
  test('replays events within kernel time window with domain filtering and max limits', () => {
    const aggregator = new CommClockAggregator();
    const coherenceEngine = new ClockCoherenceEngine();
    const rf = new MockRFClockAdapter();
    const dsp = new MockDSPClockAdapter();

    aggregator.registerRFDomain(rf);
    aggregator.registerDSPDomain(dsp);

    const replayApi = new CommReplayAPI(aggregator, coherenceEngine);

    // Populate timeline with events
    rf.fireSymbolEvent(10_000_000_000n, 1n);
    dsp.fireFrameEvent(10_500_000_000n, 1, 0);
    rf.fireSymbolEvent(11_000_000_000n, 2n);

    const kernelStart: KernelTime = {
      blockHeight: 1,
      medianBlockTime: 10n,
      mempoolTimestamp: 10_000_000_000n,
      unixNs: 10_000_000_000n,
    };

    const kernelEnd: KernelTime = {
      blockHeight: 1,
      medianBlockTime: 12n,
      mempoolTimestamp: 12_000_000_000n,
      unixNs: 12_000_000_000n,
    };

    // Replay with domain filter ['RF']
    const sessionRF = replayApi.replayCommEventsAlignedToKernel(kernelStart, kernelEnd, {
      includeDomains: ['RF'],
    });

    expect(sessionRF.totalEventsMatched).toBe(2);
    expect(sessionRF.events.every((e) => e.domainId === 'RF')).toBe(true);

    // Replay with maxEvents limit
    const sessionLimited = replayApi.replayCommEventsAlignedToKernel(kernelStart, kernelEnd, {
      maxEvents: 1,
    });

    expect(sessionLimited.totalEventsMatched).toBe(3);
    expect(sessionLimited.events.length).toBe(1);

    aggregator.shutdown();
  });
});
