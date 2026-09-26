/**
 * AILEE Trust Layer v37.0.0 - TypeScript Governance Mirror
 */

export interface ConstitutionRules {
  minQuorumThreshold: number;
  minPostureScore: number;
  minTemporalCoherence: number;
  requireOperatorSignature: boolean;
  requireSystemSignature: boolean;
  constitutionId: string;
}

export interface ApprovalFactors {
  quorumCount: number;
  totalValidators: number;
  operatorSignatureValid: boolean;
  systemSignatureValid: boolean;
  zkStateConsistent: boolean;
  postureScore: number;
  temporalCoherenceIndex: number;
  zkRecursionRoot: string;
}

export interface GateDecision {
  approved: boolean;
  quorumPassed: boolean;
  signaturesValid: boolean;
  zkValid: boolean;
  posturePassed: boolean;
  coherencePassed: boolean;
  rejectionReason?: string;
  evaluatedFactors: string[];
}

export class GovernanceMirror {
  private constitution: ConstitutionRules;

  constructor(rules?: Partial<ConstitutionRules>) {
    this.constitution = {
      minQuorumThreshold: rules?.minQuorumThreshold ?? 3,
      minPostureScore: rules?.minPostureScore ?? 2.5,
      minTemporalCoherence: rules?.minTemporalCoherence ?? 0.70,
      requireOperatorSignature: rules?.requireOperatorSignature ?? true,
      requireSystemSignature: rules?.requireSystemSignature ?? true,
      constitutionId: rules?.constitutionId ?? "v37.0.0-canonical-constitution",
    };
  }

  public getConstitution(): ConstitutionRules {
    return { ...this.constitution };
  }

  public evaluateFactors(factors: ApprovalFactors): GateDecision {
    const quorumPassed = factors.quorumCount >= this.constitution.minQuorumThreshold;
    const signaturesValid =
      (!this.constitution.requireOperatorSignature || factors.operatorSignatureValid) &&
      (!this.constitution.requireSystemSignature || factors.systemSignatureValid);
    const zkValid = factors.zkStateConsistent;
    const posturePassed = factors.postureScore <= this.constitution.minPostureScore || factors.postureScore === 0.0;
    const coherencePassed = factors.temporalCoherenceIndex >= this.constitution.minTemporalCoherence;

    const approved = quorumPassed && signaturesValid && zkValid && posturePassed && coherencePassed;

    let rejectionReason: string | undefined;
    if (!approved) {
      if (!quorumPassed) rejectionReason = "Quorum threshold not met";
      else if (!signaturesValid) rejectionReason = "Invalid signatures";
      else if (!zkValid) rejectionReason = "ZK proof state inconsistent";
      else if (!posturePassed) rejectionReason = "Posture score exceeds threshold";
      else if (!coherencePassed) rejectionReason = "Temporal coherence below threshold";
    }

    const evaluatedFactors = [
      `Quorum: ${factors.quorumCount}/${this.constitution.minQuorumThreshold} -> ${quorumPassed ? "PASS" : "FAIL"}`,
      `Signatures: ${signaturesValid ? "PASS" : "FAIL"}`,
      `ZK State: ${zkValid ? "PASS" : "FAIL"}`,
      `Posture: ${factors.postureScore} <= ${this.constitution.minPostureScore} -> ${posturePassed ? "PASS" : "FAIL"}`,
      `Coherence: ${factors.temporalCoherenceIndex} >= ${this.constitution.minTemporalCoherence} -> ${coherencePassed ? "PASS" : "FAIL"}`,
    ];

    return {
      approved,
      quorumPassed,
      signaturesValid,
      zkValid,
      posturePassed,
      coherencePassed,
      rejectionReason,
      evaluatedFactors,
    };
  }
}
