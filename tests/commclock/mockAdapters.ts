import {
  RFClockAdapter,
  DSPClockAdapter,
  NetworkClockAdapter,
} from '../../src/domains/adapters/index.js';
import {
  CommClockTimestamp,
  RFClockEvent,
  DSPClockEvent,
  NetworkClockEvent,
} from '../../src/domains/types.js';

export class MockRFClockAdapter extends RFClockAdapter {
  private phase = 0.0;
  private listeners: Set<(event: RFClockEvent) => void> = new Set();

  public getCurrentPhase(): number {
    return this.phase;
  }
  public getCarrierFrequencyHz(): number {
    return 3_500_000_000;
  }
  public getSymbolRateHz(): number {
    return 30_720_000;
  }
  public getTimestamp(): CommClockTimestamp {
    return { unixNs: BigInt(Date.now()) * 1_000_000n, domainId: 'RF', sourceId: 'MOCK_RF' };
  }
  public subscribeRFClock(callback: (event: RFClockEvent) => void): () => void {
    this.listeners.add(callback);
    return () => this.listeners.delete(callback);
  }
  public fireSymbolEvent(unixNs: bigint, symbolIndex: bigint): void {
    const event: RFClockEvent = {
      timestamp: { unixNs, domainId: 'RF', sourceId: 'MOCK_RF' },
      phaseRadians: this.phase,
      carrierFrequencyHz: 3_500_000_000,
      symbolRateHz: 30_720_000,
      symbolBoundaryIndex: symbolIndex,
    };
    for (const l of this.listeners) l(event);
  }
}

export class MockDSPClockAdapter extends DSPClockAdapter {
  private listeners: Set<(event: DSPClockEvent) => void> = new Set();

  public getBasebandClockHz(): number {
    return 122_880_000;
  }
  public getFrameNumber(): number {
    return 10;
  }
  public getSubframeIndex(): number {
    return 2;
  }
  public getTimestamp(): CommClockTimestamp {
    return { unixNs: BigInt(Date.now()) * 1_000_000n, domainId: 'DSP', sourceId: 'MOCK_DSP' };
  }
  public subscribeDSPClock(callback: (event: DSPClockEvent) => void): () => void {
    this.listeners.add(callback);
    return () => this.listeners.delete(callback);
  }
  public fireFrameEvent(unixNs: bigint, frameNumber: number, subframeIndex: number): void {
    const event: DSPClockEvent = {
      timestamp: { unixNs, domainId: 'DSP', sourceId: 'MOCK_DSP' },
      basebandClockHz: 122_880_000,
      frameNumber,
      subframeIndex,
    };
    for (const l of this.listeners) l(event);
  }
}

export class MockNetworkClockAdapter extends NetworkClockAdapter {
  private listeners: Set<(event: NetworkClockEvent) => void> = new Set();

  public getNetworkClockHz(): number {
    return 156_250_000;
  }
  public getCurrentInterfaceId(): string {
    return 'eth0';
  }
  public getCurrentPacketSequence(): number {
    return 100;
  }
  public getTimestamp(): CommClockTimestamp {
    return { unixNs: BigInt(Date.now()) * 1_000_000n, domainId: 'NETWORK', sourceId: 'MOCK_NET' };
  }
  public subscribeNetworkClock(callback: (event: NetworkClockEvent) => void): () => void {
    this.listeners.add(callback);
    return () => this.listeners.delete(callback);
  }
  public firePacketEvent(unixNs: bigint, sequence: number): void {
    const event: NetworkClockEvent = {
      timestamp: { unixNs, domainId: 'NETWORK', sourceId: 'MOCK_NET' },
      networkClockHz: 156_250_000,
      currentInterfaceId: 'eth0',
      currentPacketSequence: sequence,
    };
    for (const l of this.listeners) l(event);
  }
}
