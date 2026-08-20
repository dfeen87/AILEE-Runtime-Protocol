import { KernelTime } from '../ailee-core/types.js';
import { CommClockEvent, CommClockTimestamp } from '../domains/types.js';

/**
 * Temporal correlation between a communication clock event and a Bitcoin-aligned Kernel Time.
 */
export interface CommClockCorrelation {
  /**
   * Domain ID of the correlated clock event.
   */
  domainId: string;

  /**
   * The actual communication clock event.
   */
  commEvent: CommClockEvent;

  /**
   * Time delta in nanoseconds between KernelTime.unixNs and CommClockTimestamp.unixNs (commTime - kernelTime).
   */
  deltaNs: bigint;

  /**
   * Alignment confidence score ranging from 0.0 (unaligned) to 1.0 (perfect coherence).
   */
  confidence: number;
}

/**
 * Coherence metrics report comparing a communication clock domain against the Kernel Clock.
 */
export interface CoherenceReport {
  /**
   * Domain ID (e.g. "RF", "DSP", "NETWORK").
   */
  domainId: string;

  /**
   * Clock drift rate in nanoseconds per second.
   */
  driftNsPerSecond: number;

  /**
   * Estimated jitter in nanoseconds.
   */
  jitterNs: number;

  /**
   * Alignment confidence score between 0.0 and 1.0.
   */
  alignmentConfidence: number;

  /**
   * Timestamp of the communication clock when coherence was computed.
   */
  lastUpdated: CommClockTimestamp;
}

/**
 * Event emitted by KernelClockBridge whenever KernelClock receives an event or tick.
 */
export interface KernelCommBridgeEvent {
  /**
   * AILEE Kernel time snapshot.
   */
  kernelTime: KernelTime;

  /**
   * Correlated communication clock events around this kernel time.
   */
  correlatedCommEvents: CommClockCorrelation[];

  /**
   * Coherence reports across all monitored domains.
   */
  coherenceReports: CoherenceReport[];
}

/**
 * Options for replaying communication events aligned to kernel time.
 */
export interface CommReplayOptions {
  /**
   * Specific domain IDs to include in replay. If omitted, all domains are included.
   */
  includeDomains?: string[];

  /**
   * Maximum number of events to return in replay session.
   */
  maxEvents?: number;

  /**
   * Minimum alignment confidence threshold (0.0 to 1.0).
   */
  coherenceThreshold?: number;
}

/**
 * Result of a communication event replay session.
 */
export interface CommReplaySession {
  /**
   * Start kernel time window.
   */
  kernelStart: KernelTime;

  /**
   * End kernel time window.
   */
  kernelEnd: KernelTime;

  /**
   * List of correlated communication events in this kernel time window.
   */
  events: CommClockCorrelation[];

  /**
   * Total events found matching criteria.
   */
  totalEventsMatched: number;

  /**
   * Mean coherence confidence across replayed events.
   */
  averageConfidence: number;
}
