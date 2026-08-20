import { KernelClockAPI, KernelTimeEvent } from '../ailee-core/types.js';
import { CommClockAggregator } from './CommClockAggregator.js';
import { ClockCoherenceEngine } from './ClockCoherenceEngine.js';
import { KernelCommBridgeEvent, CoherenceReport } from './types.js';

export class KernelClockBridge {
  private readonly kernelClockApi: KernelClockAPI;
  private readonly aggregator: CommClockAggregator;
  private readonly coherenceEngine: ClockCoherenceEngine;
  private readonly listeners: Set<(event: KernelCommBridgeEvent) => void> = new Set();
  private unsubscribeKernelClock?: () => void;

  constructor(
    kernelClockApi: KernelClockAPI,
    aggregator: CommClockAggregator,
    coherenceEngine: ClockCoherenceEngine
  ) {
    this.kernelClockApi = kernelClockApi;
    this.aggregator = aggregator;
    this.coherenceEngine = coherenceEngine;

    this.startListening();
  }

  private startListening(): void {
    this.unsubscribeKernelClock = this.kernelClockApi.subscribeKernelClock((event: KernelTimeEvent) => {
      this.handleKernelEvent(event);
    });
  }

  private handleKernelEvent(event: KernelTimeEvent): void {
    const kernelTime = event.kernelTime;
    const timeline = this.aggregator.getTimeline();

    // Map communication events within 5-second window of kernel event
    const correlations = timeline.mapToKernelTime(kernelTime, 5_000_000_000n);

    // Compute or retrieve coherence reports for correlated events
    for (const corr of correlations) {
      this.coherenceEngine.computeCoherence(kernelTime, corr.commEvent.data.timestamp);
    }

    const coherenceReports: CoherenceReport[] = this.coherenceEngine.getAllLatestReports();

    const bridgeEvent: KernelCommBridgeEvent = {
      kernelTime,
      correlatedCommEvents: correlations,
      coherenceReports,
    };

    for (const listener of this.listeners) {
      listener(bridgeEvent);
    }
  }

  public subscribeKernelCommBridge(callback: (event: KernelCommBridgeEvent) => void): () => void {
    this.listeners.add(callback);
    return () => {
      this.listeners.delete(callback);
    };
  }

  public shutdown(): void {
    if (this.unsubscribeKernelClock) {
      this.unsubscribeKernelClock();
      this.unsubscribeKernelClock = undefined;
    }
    this.listeners.clear();
  }
}
