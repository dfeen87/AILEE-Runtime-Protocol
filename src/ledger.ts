/**
 * AILEE Trust Layer v37.0.0 - ALCOA Ledger Mirror
 */

export interface AlcoaEntry {
  entryId: string;
  operatorId: string;
  systemId: string;
  operatorSignature: string;
  humanReadableSummary: string;
  regimeLabel: string;
  compartmentLabel: string;
  timestampUtc: number;
  epochId: number;
  epochHash: string;
  parentEntryId?: string;
  sourceSystem: string;
  postureRegimeId: string;
  postureScore: number;
  zkRecursionRoot: string;
  temporalCoherenceIndex: number;
  signalEnergy: number;
  coherenceScore: number;
}

export class AlcoaLedgerMirror {
  private entries: AlcoaEntry[] = [];

  public recordEntry(entry: Omit<AlcoaEntry, "entryId" | "timestampUtc"> & { entryId?: string; timestampUtc?: number }): AlcoaEntry {
    const timestampUtc = entry.timestampUtc ?? Math.floor(Date.now() / 1000);
    const entryId = entry.entryId ?? `alcoa-ts-${this.entries.length + 1}-${timestampUtc}`;
    const parentEntryId = this.entries.length > 0 ? this.entries[this.entries.length - 1].entryId : undefined;

    const fullEntry: AlcoaEntry = {
      ...entry,
      entryId,
      timestampUtc,
      parentEntryId: entry.parentEntryId ?? parentEntryId,
    };

    this.entries.push(fullEntry);
    return fullEntry;
  }

  public verifyEntry(entryId: string): boolean {
    const entry = this.entries.find((e) => e.entryId === entryId);
    if (!entry) return false;

    const attributable = !!entry.operatorId && !!entry.operatorSignature;
    const legible = !!entry.humanReadableSummary && !!entry.regimeLabel;
    const contemporaneous = entry.timestampUtc > 0 && !!entry.epochHash;
    const original = !!entry.entryId;
    const accurate = !!entry.postureRegimeId;

    return attributable && legible && contemporaneous && original && accurate;
  }

  public getEntries(): AlcoaEntry[] {
    return [...this.entries];
  }

  public getLatestEntry(): AlcoaEntry | undefined {
    return this.entries[this.entries.length - 1];
  }
}
