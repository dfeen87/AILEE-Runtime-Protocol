/**
 * AILEE Trust Layer v38.0.0 - ALCOA Ledger Mirror
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

  public static computeEntryId(entry: Partial<AlcoaEntry>, parentId?: string): string {
    const raw = `${parentId || "0x00"}|${entry.operatorId}|${entry.systemId}|${entry.epochId}|${entry.postureRegimeId}`;
    let hash = 0;
    for (let i = 0; i < raw.length; i++) {
      const char = raw.charCodeAt(i);
      hash = (hash << 5) - hash + char;
      hash |= 0;
    }
    const hex = Math.abs(hash).toString(16).padStart(16, "0");
    return `alcoa-ts-0x${hex}`;
  }

  public recordEntry(entry: Omit<AlcoaEntry, "entryId" | "timestampUtc"> & { entryId?: string; timestampUtc?: number }): AlcoaEntry {
    const timestampUtc = entry.timestampUtc ?? Math.floor(Date.now() / 1000);
    const parentEntryId = entry.parentEntryId ?? (this.entries.length > 0 ? this.entries[this.entries.length - 1].entryId : "0x0000000000000000000000000000000000000000000000000000000000000000");
    const entryId = entry.entryId ?? AlcoaLedgerMirror.computeEntryId(entry, parentEntryId);

    const fullEntry: AlcoaEntry = {
      ...entry,
      entryId,
      timestampUtc,
      parentEntryId,
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
    const accurate = !!entry.postureRegimeId && entry.temporalCoherenceIndex >= 0.0 && entry.temporalCoherenceIndex <= 1.0;

    return attributable && legible && contemporaneous && original && accurate;
  }

  public verifyChain(): boolean {
    if (this.entries.length === 0) return true;

    for (let i = 0; i < this.entries.length; i++) {
      const entry = this.entries[i];
      if (!this.verifyEntry(entry.entryId)) return false;

      if (i > 0) {
        if (entry.parentEntryId !== this.entries[i - 1].entryId) {
          return false; // Parent link broken
        }
      }
    }
    return true;
  }

  public getEntries(): AlcoaEntry[] {
    return [...this.entries];
  }

  public getLatestEntry(): AlcoaEntry | undefined {
    return this.entries[this.entries.length - 1];
  }
}
