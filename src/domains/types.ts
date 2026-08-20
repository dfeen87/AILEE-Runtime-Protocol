/**
 * Shared Timestamp and Domain Interfaces for Communication-Grade Clock Domains.
 */

/**
 * Shared high-precision timestamp used across all communication clock domains.
 */
export interface CommClockTimestamp {
  /**
   * Nanoseconds since UNIX epoch (bigint for 64-bit precision).
   */
  unixNs: bigint;

  /**
   * Identifies clock domain, e.g., "RF", "MIXED_SIGNAL", "DSP", "SECURITY", "COMPRESSION", "NETWORK", "POWER".
   */
  domainId: string;

  /**
   * Hardware or logical source identifier (e.g. "MAXLINEAR_RF_0", "ETH_PHY_1").
   */
  sourceId: string;
}

/**
 * Power management states for communication hardware.
 */
export type PowerState = 'FULL_POWER' | 'LOW_POWER' | 'CLOCK_GATED' | 'SLEEP' | 'DEEP_SLEEP';

// --- Domain Events ---

export interface RFClockEvent {
  timestamp: CommClockTimestamp;
  phaseRadians: number;
  carrierFrequencyHz: number;
  symbolRateHz: number;
  symbolBoundaryIndex: bigint;
}

export interface MixedSignalClockEvent {
  timestamp: CommClockTimestamp;
  adcSamplingRateHz: number;
  dacUpdateRateHz: number;
  pipelineLatencyNs: number;
  sampleBatchIndex: bigint;
}

export interface DSPClockEvent {
  timestamp: CommClockTimestamp;
  basebandClockHz: number;
  frameNumber: number;
  subframeIndex: number;
}

export interface SecurityClockEvent {
  timestamp: CommClockTimestamp;
  securityClockHz: number;
  lastOperationId: string;
}

export interface CompressionClockEvent {
  timestamp: CommClockTimestamp;
  compressionClockHz: number;
  currentStreamId: string;
}

export interface NetworkClockEvent {
  timestamp: CommClockTimestamp;
  networkClockHz: number;
  currentInterfaceId: string;
  currentPacketSequence: number;
}

export interface PowerClockEvent {
  timestamp: CommClockTimestamp;
  powerClockHz: number;
  currentPowerState: PowerState;
}

/**
 * Unified communication clock event union type.
 */
export type CommClockEvent =
  | { domainId: 'RF'; data: RFClockEvent }
  | { domainId: 'MIXED_SIGNAL'; data: MixedSignalClockEvent }
  | { domainId: 'DSP'; data: DSPClockEvent }
  | { domainId: 'SECURITY'; data: SecurityClockEvent }
  | { domainId: 'COMPRESSION'; data: CompressionClockEvent }
  | { domainId: 'NETWORK'; data: NetworkClockEvent }
  | { domainId: 'POWER'; data: PowerClockEvent };

// --- Domain Interfaces ---

/**
 * 2.1 RF / Radio Frequency Clock Domain Interface.
 */
export interface RFClockDomain {
  getCurrentPhase(): number; // radians
  getCarrierFrequencyHz(): number;
  getSymbolRateHz(): number;
  getTimestamp(): CommClockTimestamp;
  subscribeRFClock(callback: (event: RFClockEvent) => void): () => void;
}

/**
 * 2.2 High-Performance Analog / Mixed-Signal Clock Domain Interface.
 */
export interface MixedSignalClockDomain {
  getAdcSamplingRateHz(): number;
  getDacUpdateRateHz(): number;
  getPipelineLatencyNs(): number;
  getTimestamp(): CommClockTimestamp;
  subscribeMixedSignalClock(callback: (event: MixedSignalClockEvent) => void): () => void;
}

/**
 * 2.3 DSP / Digital Signal Processing Clock Domain Interface.
 */
export interface DSPClockDomain {
  getBasebandClockHz(): number;
  getFrameNumber(): number;
  getSubframeIndex(): number;
  getTimestamp(): CommClockTimestamp;
  subscribeDSPClock(callback: (event: DSPClockEvent) => void): () => void;
}

/**
 * 2.4 Security Engine Clock Domain Interface.
 */
export interface SecurityClockDomain {
  getSecurityClockHz(): number;
  getLastOperationId(): string;
  getTimestamp(): CommClockTimestamp;
  subscribeSecurityClock(callback: (event: SecurityClockEvent) => void): () => void;
}

/**
 * 2.5 Data Compression Clock Domain Interface.
 */
export interface CompressionClockDomain {
  getCompressionClockHz(): number;
  getCurrentStreamId(): string;
  getTimestamp(): CommClockTimestamp;
  subscribeCompressionClock(callback: (event: CompressionClockEvent) => void): () => void;
}

/**
 * 2.6 Networking Layer Clock Domain Interface.
 */
export interface NetworkClockDomain {
  getNetworkClockHz(): number;
  getCurrentInterfaceId(): string;
  getCurrentPacketSequence(): number;
  getTimestamp(): CommClockTimestamp;
  subscribeNetworkClock(callback: (event: NetworkClockEvent) => void): () => void;
}

/**
 * 2.7 Power Management Clock Domain Interface.
 */
export interface PowerClockDomain {
  getPowerClockHz(): number;
  getCurrentPowerState(): PowerState;
  getTimestamp(): CommClockTimestamp;
  subscribePowerClock(callback: (event: PowerClockEvent) => void): () => void;
}
