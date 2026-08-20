import { NetworkClockAdapter } from '../../adapters/index.js';
import { CommClockTimestamp, NetworkClockEvent } from '../../types.js';

export interface MaxLinearNetworkDriverOptions {
  sourceId?: string;
  interfaceId?: string;
  networkClockHz?: number;
}

/**
 * Example Stub Implementation of Network Clock Adapter for MaxLinear Gateway / Ethernet / PON hardware.
 */
export class MaxLinearNetworkClockAdapter extends NetworkClockAdapter {
  private readonly sourceId: string;
  private readonly interfaceId: string;
  private readonly networkClockHz: number;
  private packetSequence = 0;
  private readonly listeners: Set<(event: NetworkClockEvent) => void> = new Set();

  constructor(options: MaxLinearNetworkDriverOptions = {}) {
    super();
    this.sourceId = options.sourceId ?? 'MXL_NET_ENGINE_0';
    this.interfaceId = options.interfaceId ?? 'eth0';
    this.networkClockHz = options.networkClockHz ?? 156_250_000; // 156.25 MHz Ethernet PHY clock
  }

  public getNetworkClockHz(): number {
    // TODO: Read IEEE 1588 / PTP / Ethernet PHY reference clock speed.
    return this.networkClockHz;
  }

  public getCurrentInterfaceId(): string {
    return this.interfaceId;
  }

  public getCurrentPacketSequence(): number {
    // TODO: Query hardware MAC packet counter register.
    return this.packetSequence;
  }

  public getTimestamp(): CommClockTimestamp {
    // TODO: Read IEEE 1588 PTP hardware timestamping register.
    return {
      unixNs: BigInt(Date.now()) * 1_000_000n,
      domainId: 'NETWORK',
      sourceId: this.sourceId,
    };
  }

  public subscribeNetworkClock(callback: (event: NetworkClockEvent) => void): () => void {
    // TODO: Connect to network packet timestamping interrupt / rx buffer callback.
    this.listeners.add(callback);
    return () => {
      this.listeners.delete(callback);
    };
  }

  public emitHardwareEvent(event: NetworkClockEvent): void {
    this.packetSequence = event.currentPacketSequence;
    for (const listener of this.listeners) {
      listener(event);
    }
  }
}
