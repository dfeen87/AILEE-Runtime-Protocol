import { KernelTime } from '../ailee-core/types.js';
import {
  CommClockEvent,
  CommClockTimestamp,
  RFClockDomain,
  MixedSignalClockDomain,
  DSPClockDomain,
  SecurityClockDomain,
  CompressionClockDomain,
  NetworkClockDomain,
  PowerClockDomain,
} from '../domains/types.js';
import { CommClockCorrelation } from './types.js';

/**
 * Unified timeline storing, indexing, and correlating multi-domain communication clock events.
 */
export class UnifiedCommTimeline {
  private events: CommClockEvent[] = [];
  private readonly maxBufferSize: number;

  constructor(maxBufferSize = 10_000) {
    this.maxBufferSize = maxBufferSize;
  }

  /**
   * Helper to extract timestamp from a CommClockEvent.
   */
  public static getEventTimestamp(event: CommClockEvent): CommClockTimestamp {
    return event.data.timestamp;
  }

  /**
   * Add a new communication event to the timeline.
   */
  public addEvent(domainId: string, event: CommClockEvent): void {
    if (event.domainId !== domainId) {
      throw new Error(`Domain mismatch: event domain '${event.domainId}' does not match parameter '${domainId}'`);
    }
    this.events.push(event);

    // Keep buffer bounded
    if (this.events.length > this.maxBufferSize) {
      this.events.splice(0, this.events.length - this.maxBufferSize);
    }
  }

  /**
   * Query events whose timestamp falls between start and end (inclusive).
   */
  public getEventsBetween(start: CommClockTimestamp, end: CommClockTimestamp): CommClockEvent[] {
    const startNs = start.unixNs;
    const endNs = end.unixNs;

    return this.events.filter((e) => {
      const tsNs = e.data.timestamp.unixNs;
      return tsNs >= startNs && tsNs <= endNs;
    });
  }

  /**
   * Map communication clock events to a given Bitcoin KernelTime snapshot.
   */
  public mapToKernelTime(kernelTime: KernelTime, windowNs: bigint = 1_000_000_000n): CommClockCorrelation[] {
    const kNs = kernelTime.unixNs;
    const correlations: CommClockCorrelation[] = [];

    for (const event of this.events) {
      const commNs = event.data.timestamp.unixNs;
      const deltaNs = commNs - kNs;
      const absDelta = deltaNs < 0n ? -deltaNs : deltaNs;

      if (absDelta <= windowNs) {
        // Compute alignment confidence score: 1.0 when delta is 0, decaying as absDelta approaches windowNs
        const windowFloat = Number(windowNs);
        const deltaFloat = Number(absDelta);
        const confidence = Math.max(0, Math.min(1, 1 - deltaFloat / windowFloat));

        correlations.push({
          domainId: event.domainId,
          commEvent: event,
          deltaNs,
          confidence,
        });
      }
    }

    return correlations;
  }

  /**
   * Clear all buffered events.
   */
  public clear(): void {
    this.events = [];
  }

  /**
   * Get total event count in timeline.
   */
  public get length(): number {
    return this.events.length;
  }
}

/**
 * Multi-Domain Clock Aggregator subscribing to clock domains and updating UnifiedCommTimeline.
 */
export class CommClockAggregator {
  private readonly timeline: UnifiedCommTimeline;
  private readonly unsubscribers: Array<() => void> = [];
  private readonly activeDomains: Set<string> = new Set();

  constructor(maxBufferSize = 10_000) {
    this.timeline = new UnifiedCommTimeline(maxBufferSize);
  }

  public getTimeline(): UnifiedCommTimeline {
    return this.timeline;
  }

  public getActiveDomainIds(): string[] {
    return Array.from(this.activeDomains);
  }

  // --- Domain Subscriptions ---

  public registerRFDomain(domain: RFClockDomain): void {
    const unsub = domain.subscribeRFClock((event) => {
      this.timeline.addEvent('RF', { domainId: 'RF', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('RF');
  }

  public registerMixedSignalDomain(domain: MixedSignalClockDomain): void {
    const unsub = domain.subscribeMixedSignalClock((event) => {
      this.timeline.addEvent('MIXED_SIGNAL', { domainId: 'MIXED_SIGNAL', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('MIXED_SIGNAL');
  }

  public registerDSPDomain(domain: DSPClockDomain): void {
    const unsub = domain.subscribeDSPClock((event) => {
      this.timeline.addEvent('DSP', { domainId: 'DSP', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('DSP');
  }

  public registerSecurityDomain(domain: SecurityClockDomain): void {
    const unsub = domain.subscribeSecurityClock((event) => {
      this.timeline.addEvent('SECURITY', { domainId: 'SECURITY', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('SECURITY');
  }

  public registerCompressionDomain(domain: CompressionClockDomain): void {
    const unsub = domain.subscribeCompressionClock((event) => {
      this.timeline.addEvent('COMPRESSION', { domainId: 'COMPRESSION', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('COMPRESSION');
  }

  public registerNetworkDomain(domain: NetworkClockDomain): void {
    const unsub = domain.subscribeNetworkClock((event) => {
      this.timeline.addEvent('NETWORK', { domainId: 'NETWORK', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('NETWORK');
  }

  public registerPowerDomain(domain: PowerClockDomain): void {
    const unsub = domain.subscribePowerClock((event) => {
      this.timeline.addEvent('POWER', { domainId: 'POWER', data: event });
    });
    this.unsubscribers.push(unsub);
    this.activeDomains.add('POWER');
  }

  /**
   * Stop all active domain subscriptions.
   */
  public shutdown(): void {
    for (const unsub of this.unsubscribers) {
      unsub();
    }
    this.unsubscribers.length = 0;
    this.activeDomains.clear();
  }
}
