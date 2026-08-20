import { UnifiedCommTimeline, CommClockAggregator } from '../../src/core/CommClockAggregator.js';
import { KernelTime } from '../../src/ailee-core/types.js';
import { MockRFClockAdapter, MockDSPClockAdapter, MockNetworkClockAdapter } from './mockAdapters.js';

describe('CommClockAggregator & UnifiedCommTimeline', () => {
  test('UnifiedCommTimeline buffers and queries events between timestamps correctly', () => {
    const timeline = new UnifiedCommTimeline(100);

    const ts1 = { unixNs: 1_000_000_000n, domainId: 'RF', sourceId: 'src1' };
    const ts2 = { unixNs: 2_000_000_000n, domainId: 'RF', sourceId: 'src1' };
    const ts3 = { unixNs: 3_000_000_000n, domainId: 'RF', sourceId: 'src1' };

    timeline.addEvent('RF', {
      domainId: 'RF',
      data: {
        timestamp: ts1,
        phaseRadians: 0,
        carrierFrequencyHz: 1e9,
        symbolRateHz: 1e6,
        symbolBoundaryIndex: 1n,
      },
    });

    timeline.addEvent('RF', {
      domainId: 'RF',
      data: {
        timestamp: ts2,
        phaseRadians: 0,
        carrierFrequencyHz: 1e9,
        symbolRateHz: 1e6,
        symbolBoundaryIndex: 2n,
      },
    });

    timeline.addEvent('RF', {
      domainId: 'RF',
      data: {
        timestamp: ts3,
        phaseRadians: 0,
        carrierFrequencyHz: 1e9,
        symbolRateHz: 1e6,
        symbolBoundaryIndex: 3n,
      },
    });

    expect(timeline.length).toBe(3);

    const queried = timeline.getEventsBetween(
      { unixNs: 1_500_000_000n, domainId: 'RF', sourceId: 'query' },
      { unixNs: 2_500_000_000n, domainId: 'RF', sourceId: 'query' }
    );

    expect(queried.length).toBe(1);
    expect(queried[0].data.timestamp.unixNs).toBe(2_000_000_000n);
  });

  test('UnifiedCommTimeline maps events to kernel time with confidence calculation', () => {
    const timeline = new UnifiedCommTimeline();
    const commNs = 1_000_000_000n;

    timeline.addEvent('RF', {
      domainId: 'RF',
      data: {
        timestamp: { unixNs: commNs, domainId: 'RF', sourceId: 'src1' },
        phaseRadians: 0,
        carrierFrequencyHz: 1e9,
        symbolRateHz: 1e6,
        symbolBoundaryIndex: 1n,
      },
    });

    const kernelTimeExact: KernelTime = {
      blockHeight: 100,
      medianBlockTime: 1000n,
      mempoolTimestamp: commNs,
      unixNs: commNs,
    };

    const correlationsExact = timeline.mapToKernelTime(kernelTimeExact, 1_000_000_000n);
    expect(correlationsExact.length).toBe(1);
    expect(correlationsExact[0].deltaNs).toBe(0n);
    expect(correlationsExact[0].confidence).toBe(1.0);

    const kernelTimeOffset: KernelTime = {
      blockHeight: 100,
      medianBlockTime: 1000n,
      mempoolTimestamp: commNs - 500_000_000n,
      unixNs: commNs - 500_000_000n,
    };

    const correlationsOffset = timeline.mapToKernelTime(kernelTimeOffset, 1_000_000_000n);
    expect(correlationsOffset.length).toBe(1);
    expect(correlationsOffset[0].deltaNs).toBe(500_000_000n);
    expect(correlationsOffset[0].confidence).toBeCloseTo(0.5, 4);
  });

  test('CommClockAggregator subscribes to multiple clock domains', () => {
    const aggregator = new CommClockAggregator();
    const rf = new MockRFClockAdapter();
    const dsp = new MockDSPClockAdapter();
    const net = new MockNetworkClockAdapter();

    aggregator.registerRFDomain(rf);
    aggregator.registerDSPDomain(dsp);
    aggregator.registerNetworkDomain(net);

    expect(aggregator.getActiveDomainIds()).toEqual(['RF', 'DSP', 'NETWORK']);

    rf.fireSymbolEvent(100n, 1n);
    dsp.fireFrameEvent(200n, 0, 0);
    net.firePacketEvent(300n, 1);

    expect(aggregator.getTimeline().length).toBe(3);

    aggregator.shutdown();
    expect(aggregator.getActiveDomainIds().length).toBe(0);
  });
});
