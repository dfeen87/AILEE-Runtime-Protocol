import { KernelTime } from '../ailee-core/types.js';
import { CommClockAggregator } from './CommClockAggregator.js';
import { ClockCoherenceEngine } from './ClockCoherenceEngine.js';
import { CommReplayOptions, CommReplaySession, CommClockCorrelation } from './types.js';

export interface CommReplayAPIInterface {
  replayCommEventsAlignedToKernel(
    kernelStart: KernelTime,
    kernelEnd: KernelTime,
    options?: CommReplayOptions
  ): CommReplaySession;
}

export class CommReplayAPI implements CommReplayAPIInterface {
  private readonly aggregator: CommClockAggregator;
  private readonly coherenceEngine: ClockCoherenceEngine;

  constructor(aggregator: CommClockAggregator, coherenceEngine: ClockCoherenceEngine) {
    this.aggregator = aggregator;
    this.coherenceEngine = coherenceEngine;
  }

  public replayCommEventsAlignedToKernel(
    kernelStart: KernelTime,
    kernelEnd: KernelTime,
    options: CommReplayOptions = {}
  ): CommReplaySession {
    const timeline = this.aggregator.getTimeline();

    // Span from start to end in nanoseconds
    const startNs = kernelStart.unixNs;
    const endNs = kernelEnd.unixNs;
    const midNs = startNs + (endNs - startNs) / 2n;

    // Synthetic mid kernel time for mapping
    const midKernelTime: KernelTime = {
      blockHeight: Math.floor((kernelStart.blockHeight + kernelEnd.blockHeight) / 2),
      medianBlockTime: kernelStart.medianBlockTime,
      mempoolTimestamp: kernelStart.mempoolTimestamp,
      unixNs: midNs,
    };

    const windowNs = (endNs - startNs) / 2n;
    let correlations = timeline.mapToKernelTime(midKernelTime, windowNs);

    // Filter by includeDomains if provided
    if (options.includeDomains && options.includeDomains.length > 0) {
      const allowed = new Set(options.includeDomains);
      correlations = correlations.filter((c) => allowed.has(c.domainId));
    }

    // Filter by coherenceThreshold if provided
    if (options.coherenceThreshold !== undefined) {
      const minConfidence = options.coherenceThreshold;
      correlations = correlations.filter((c) => {
        const report = this.coherenceEngine.getCurrentCoherence(c.domainId);
        // Fallback to c.confidence if domain report unavailable
        const conf = report.alignmentConfidence > 0 ? report.alignmentConfidence : c.confidence;
        return conf >= minConfidence;
      });
    }

    const totalEventsMatched = correlations.length;

    // Apply maxEvents limit if provided
    if (options.maxEvents !== undefined && options.maxEvents > 0) {
      correlations = correlations.slice(0, options.maxEvents);
    }

    // Compute average confidence
    const sumConfidence = correlations.reduce((acc, curr) => acc + curr.confidence, 0);
    const averageConfidence = correlations.length > 0 ? sumConfidence / correlations.length : 0;

    return {
      kernelStart,
      kernelEnd,
      events: correlations,
      totalEventsMatched,
      averageConfidence,
    };
  }
}
