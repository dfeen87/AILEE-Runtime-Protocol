/**
 * AILEE Trust Layer v37.0.0 - Compartment State Machine Mirror
 */

export enum CompartmentState {
  ISOLATED = "ISOLATED",
  MONITORED = "MONITORED",
  ACTIVE = "ACTIVE",
  SUSPENDED = "SUSPENDED",
  QUARANTINED = "QUARANTINED",
}

export interface CompartmentInfo {
  compartmentId: string;
  name: string;
  state: CompartmentState;
  lastTransitionTimestamp: number;
  isolationPolicy: string;
}

export class CompartmentStateMachineMirror {
  private compartments: Map<string, CompartmentInfo> = new Map();

  constructor() {
    this.registerCompartment("core-execution", "Core Execution Engine");
    this.registerCompartment("governance-gate", "Governance Approval Gate");
    this.registerCompartment("alcoa-ledger", "ALCOA Ledger Subsystem");
    this.registerCompartment("network-relay", "P2P Network & Relay");

    this.transitionState("core-execution", CompartmentState.ACTIVE, "System initialization");
    this.transitionState("governance-gate", CompartmentState.ACTIVE, "System initialization");
    this.transitionState("alcoa-ledger", CompartmentState.ACTIVE, "System initialization");
    this.transitionState("network-relay", CompartmentState.MONITORED, "System initialization");
  }

  public registerCompartment(id: string, name: string): boolean {
    if (this.compartments.has(id)) return false;

    this.compartments.set(id, {
      compartmentId: id,
      name,
      state: CompartmentState.ISOLATED,
      lastTransitionTimestamp: Math.floor(Date.now() / 1000),
      isolationPolicy: "default-strict-isolation",
    });
    return true;
  }

  public transitionState(id: string, newState: CompartmentState, reason?: string): boolean {
    const comp = this.compartments.get(id);
    if (!comp) return false;

    comp.state = newState;
    comp.lastTransitionTimestamp = Math.floor(Date.now() / 1000);
    if (reason) comp.isolationPolicy = reason;
    return true;
  }

  public getCompartment(id: string): CompartmentInfo | undefined {
    return this.compartments.get(id);
  }

  public listCompartments(): CompartmentInfo[] {
    return Array.from(this.compartments.values());
  }

  public isIsolated(id: string): boolean {
    const comp = this.compartments.get(id);
    if (!comp) return true;
    return comp.state === CompartmentState.ISOLATED || comp.state === CompartmentState.QUARANTINED;
  }
}
