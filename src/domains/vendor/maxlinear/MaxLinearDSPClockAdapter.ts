import { DSPClockAdapter } from '../../adapters/index.js';
import { CommClockTimestamp, DSPClockEvent } from '../../types.js';

export interface MaxLinearDSPDriverOptions {
  sourceId?: string;
  basebandClockHz?: number;
}

/**
 * Example Stub Implementation of DSP Clock Adapter for MaxLinear Baseband Processors.
 */
export class MaxLinearDSPClockAdapter extends DSPClockAdapter {
  private readonly sourceId: string;
  private readonly basebandClockHz: number;
  private currentFrame = 0;
  private currentSubframe = 0;
  private readonly listeners: Set<(event: DSPClockEvent) => void> = new Set();

  constructor(options: MaxLinearDSPDriverOptions = {}) {
    super();
    this.sourceId = options.sourceId ?? 'MXL_DSP_BASEBAND_0';
    this.basebandClockHz = options.basebandClockHz ?? 122_880_000; // 122.88 MHz baseband
  }

  public getBasebandClockHz(): number {
    // TODO: Query MaxLinear DSP core clock management unit (CMU).
    return this.basebandClockHz;
  }

  public getFrameNumber(): number {
    // TODO: Read 3GPP LTE/5G System Frame Number (SFN) counter register (0..1023).
    return this.currentFrame;
  }

  public getSubframeIndex(): number {
    // TODO: Read Subframe/Slot counter register (0..9 or 0..19).
    return this.currentSubframe;
  }

  public getTimestamp(): CommClockTimestamp {
    // TODO: Convert DSP hardware frame timer counter to UNIX nanoseconds.
    return {
      unixNs: BigInt(Date.now()) * 1_000_000n,
      domainId: 'DSP',
      sourceId: this.sourceId,
    };
  }

  public subscribeDSPClock(callback: (event: DSPClockEvent) => void): () => void {
    // TODO: Connect to MaxLinear DSP slot timer interrupt.
    this.listeners.add(callback);
    return () => {
      this.listeners.delete(callback);
    };
  }

  public emitHardwareEvent(event: DSPClockEvent): void {
    this.currentFrame = event.frameNumber;
    this.currentSubframe = event.subframeIndex;
    for (const listener of this.listeners) {
      listener(event);
    }
  }
}
