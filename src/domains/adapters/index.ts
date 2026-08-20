import {
  RFClockDomain,
  MixedSignalClockDomain,
  DSPClockDomain,
  SecurityClockDomain,
  CompressionClockDomain,
  NetworkClockDomain,
  PowerClockDomain,
  CommClockTimestamp,
  RFClockEvent,
  MixedSignalClockEvent,
  DSPClockEvent,
  SecurityClockEvent,
  CompressionClockEvent,
  NetworkClockEvent,
  PowerClockEvent,
} from '../types.js';

/**
 * Abstract Base Class for RF Clock Adapters.
 */
export abstract class RFClockAdapter implements RFClockDomain {
  abstract getCurrentPhase(): number;
  abstract getCarrierFrequencyHz(): number;
  abstract getSymbolRateHz(): number;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribeRFClock(callback: (event: RFClockEvent) => void): () => void;
}

/**
 * Abstract Base Class for Mixed-Signal Clock Adapters.
 */
export abstract class MixedSignalClockAdapter implements MixedSignalClockDomain {
  abstract getAdcSamplingRateHz(): number;
  abstract getDacUpdateRateHz(): number;
  abstract getPipelineLatencyNs(): number;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribeMixedSignalClock(callback: (event: MixedSignalClockEvent) => void): () => void;
}

/**
 * Abstract Base Class for DSP Clock Adapters.
 */
export abstract class DSPClockAdapter implements DSPClockDomain {
  abstract getBasebandClockHz(): number;
  abstract getFrameNumber(): number;
  abstract getSubframeIndex(): number;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribeDSPClock(callback: (event: DSPClockEvent) => void): () => void;
}

/**
 * Abstract Base Class for Security Engine Clock Adapters.
 */
export abstract class SecurityClockAdapter implements SecurityClockDomain {
  abstract getSecurityClockHz(): number;
  abstract getLastOperationId(): string;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribeSecurityClock(callback: (event: SecurityClockEvent) => void): () => void;
}

/**
 * Abstract Base Class for Compression Engine Clock Adapters.
 */
export abstract class CompressionClockAdapter implements CompressionClockDomain {
  abstract getCompressionClockHz(): number;
  abstract getCurrentStreamId(): string;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribeCompressionClock(callback: (event: CompressionClockEvent) => void): () => void;
}

/**
 * Abstract Base Class for Network Clock Adapters.
 */
export abstract class NetworkClockAdapter implements NetworkClockDomain {
  abstract getNetworkClockHz(): number;
  abstract getCurrentInterfaceId(): string;
  abstract getCurrentPacketSequence(): number;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribeNetworkClock(callback: (event: NetworkClockEvent) => void): () => void;
}

/**
 * Abstract Base Class for Power Management Clock Adapters.
 */
export abstract class PowerClockAdapter implements PowerClockDomain {
  abstract getPowerClockHz(): number;
  abstract getCurrentPowerState(): import('../types.js').PowerState;
  abstract getTimestamp(): CommClockTimestamp;
  abstract subscribePowerClock(callback: (event: PowerClockEvent) => void): () => void;
}
