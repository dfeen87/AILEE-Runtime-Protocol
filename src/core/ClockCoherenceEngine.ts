import { KernelTime } from '../ailee-core/types.js';
import { CommClockTimestamp } from '../domains/types.js';
import { CoherenceReport } from './types.js';

export interface ClockCoherenceEngineAPI {
  computeCoherence(kernelTime: KernelTime, commTimestamp: CommClockTimestamp): CoherenceReport;
  getCurrentCoherence(domainId: string): CoherenceReport;
  subscribeCoherenceUpdates(callback: (report: CoherenceReport) => void): () => void;
}

interface DomainHistoryPoint {
  kernelNs: bigint;
  commNs: bigint;
  deltaNs: number;
}

/**
 * Clock Coherence Engine computing drift, jitter, and alignment confidence across clock domains.
 */
export class ClockCoherenceEngine implements ClockCoherenceEngineAPI {
  private readonly historyMap: Map<string, DomainHistoryPoint[]> = new Map();
  private readonly latestReports: Map<string, CoherenceReport> = new Map();
  private readonly listeners: Set<(report: CoherenceReport) => void> = new Set();
  private readonly historyWindowSize: number;

  constructor(historyWindowSize = 50) {
    this.historyWindowSize = historyWindowSize;
  }

  public computeCoherence(kernelTime: KernelTime, commTimestamp: CommClockTimestamp): CoherenceReport {
    const domainId = commTimestamp.domainId;
    const kernelNs = kernelTime.unixNs;
    const commNs = commTimestamp.unixNs;
    const deltaNs = Number(commNs - kernelNs);

    let history = this.historyMap.get(domainId);
    if (!history) {
      history = [];
      this.historyMap.set(domainId, history);
    }

    history.push({ kernelNs, commNs, deltaNs });
    if (history.length > this.historyWindowSize) {
      history.shift();
    }

    let driftNsPerSecond = 0;
    let jitterNs = 0;
    let alignmentConfidence = 1.0;

    if (history.length >= 2) {
      const first = history[0];
      const last = history[history.length - 1];

      const elapsedKernelSeconds = Number(last.kernelNs - first.kernelNs) / 1e9;
      if (elapsedKernelSeconds > 0) {
        const driftTotalNs = last.deltaNs - first.deltaNs;
        driftNsPerSecond = driftTotalNs / elapsedKernelSeconds;
      }

      // Compute jitter as standard deviation of deltaNs in window
      const meanDelta = history.reduce((sum, p) => sum + p.deltaNs, 0) / history.length;
      const variance = history.reduce((sum, p) => sum + Math.pow(p.deltaNs - meanDelta, 2), 0) / history.length;
      jitterNs = Math.sqrt(variance);

      // Alignment confidence model: based on absolute delta and jitter
      const absDelta = Math.abs(deltaNs);
      const deltaPenalty = Math.min(1.0, absDelta / 1e9); // 1 sec delta = max penalty
      const jitterPenalty = Math.min(1.0, jitterNs / 1e6); // 1 ms jitter = max penalty

      alignmentConfidence = Math.max(0, 1.0 - 0.7 * deltaPenalty - 0.3 * jitterPenalty);
    } else {
      // Single sample baseline
      const absDelta = Math.abs(deltaNs);
      alignmentConfidence = Math.max(0, 1.0 - Math.min(1.0, absDelta / 1e9));
    }

    const report: CoherenceReport = {
      domainId,
      driftNsPerSecond,
      jitterNs,
      alignmentConfidence,
      lastUpdated: commTimestamp,
    };

    this.latestReports.set(domainId, report);

    for (const listener of this.listeners) {
      listener(report);
    }

    return report;
  }

  public getCurrentCoherence(domainId: string): CoherenceReport {
    const report = this.latestReports.get(domainId);
    if (!report) {
      return {
        domainId,
        driftNsPerSecond: 0,
        jitterNs: 0,
        alignmentConfidence: 0.0,
        lastUpdated: {
          unixNs: 0n,
          domainId,
          sourceId: 'UNINITIALIZED',
        },
      };
    }
    return report;
  }

  public subscribeCoherenceUpdates(callback: (report: CoherenceReport) => void): () => void {
    this.listeners.add(callback);
    return () => {
      this.listeners.delete(callback);
    };
  }

  public getAllLatestReports(): CoherenceReport[] {
    return Array.from(this.latestReports.values());
  }
}
