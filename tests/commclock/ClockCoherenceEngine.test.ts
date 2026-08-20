import { ClockCoherenceEngine } from '../../src/core/ClockCoherenceEngine.js';
import { KernelTime } from '../../src/ailee-core/types.js';
import { CommClockTimestamp } from '../../src/domains/types.js';

describe('ClockCoherenceEngine', () => {
  test('computes baseline coherence report on first sample', () => {
    const engine = new ClockCoherenceEngine();

    const kernelTime: KernelTime = {
      blockHeight: 800_000,
      medianBlockTime: 1_700_000_000n,
      mempoolTimestamp: 1_700_000_000_000_000_000n,
      unixNs: 1_700_000_000_000_000_000n,
    };

    const commTs: CommClockTimestamp = {
      unixNs: 1_700_000_000_000_000_000n,
      domainId: 'RF',
      sourceId: 'MXL_RF',
    };

    const report = engine.computeCoherence(kernelTime, commTs);

    expect(report.domainId).toBe('RF');
    expect(report.driftNsPerSecond).toBe(0);
    expect(report.jitterNs).toBe(0);
    expect(report.alignmentConfidence).toBe(1.0);
  });

  test('computes drift and jitter over time window', () => {
    const engine = new ClockCoherenceEngine(10);

    const baseKernelNs = 1_000_000_000_000n; // 1000s

    // Simulate 5 consecutive samples spaced 1s apart with slight drift (+10ns/s)
    for (let i = 0; i < 5; i++) {
      const kNs = baseKernelNs + BigInt(i) * 1_000_000_000n;
      const cNs = kNs + BigInt(i * 10); // 10ns incremental drift each step

      const kt: KernelTime = {
        blockHeight: 800_000 + i,
        medianBlockTime: kNs / 1_000_000_000n,
        mempoolTimestamp: kNs,
        unixNs: kNs,
      };

      const ct: CommClockTimestamp = {
        unixNs: cNs,
        domainId: 'DSP',
        sourceId: 'DSP_0',
      };

      engine.computeCoherence(kt, ct);
    }

    const report = engine.getCurrentCoherence('DSP');
    expect(report.domainId).toBe('DSP');
    expect(report.driftNsPerSecond).toBeCloseTo(10, 1);
    expect(report.jitterNs).toBeGreaterThan(0);
    expect(report.alignmentConfidence).toBeGreaterThan(0.9);
  });

  test('notifies subscribers on coherence updates', () => {
    const engine = new ClockCoherenceEngine();
    const reports: any[] = [];

    const unsub = engine.subscribeCoherenceUpdates((r) => reports.push(r));

    const kt: KernelTime = {
      blockHeight: 1,
      medianBlockTime: 100n,
      mempoolTimestamp: 1000n,
      unixNs: 1000n,
    };
    const ct: CommClockTimestamp = {
      unixNs: 1000n,
      domainId: 'NETWORK',
      sourceId: 'NET_0',
    };

    engine.computeCoherence(kt, ct);
    expect(reports.length).toBe(1);
    expect(reports[0].domainId).toBe('NETWORK');

    unsub();
    engine.computeCoherence(kt, ct);
    expect(reports.length).toBe(1);
  });
});
