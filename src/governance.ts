/**
 * AILEE Trust Layer v37.1.0 - TypeScript Governance Mirror
 */

export interface ConstitutionRules {
  minQuorumThreshold: number;
  maxAllowablePostureScore: number;
  minPostureScore: number; // Backward compatibility alias
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
    const maxScore = rules?.maxAllowablePostureScore ?? rules?.minPostureScore ?? 2.5;
    this.constitution = {
      minQuorumThreshold: rules?.minQuorumThreshold ?? 3,
      maxAllowablePostureScore: maxScore,
      minPostureScore: maxScore,
      minTemporalCoherence: rules?.minTemporalCoherence ?? 0.70,
      requireOperatorSignature: rules?.requireOperatorSignature ?? true,
      requireSystemSignature: rules?.requireSystemSignature ?? true,
      constitutionId: rules?.constitutionId ?? "v37.1.0-canonical-constitution",
    };
  }

  public getConstitution(): ConstitutionRules {
    return { ...this.constitution };
  }

  public evaluateFactors(factors: ApprovalFactors): GateDecision {
    const quorumPassed =
      factors.quorumCount >= this.constitution.minQuorumThreshold &&
      factors.totalValidators >= this.constitution.minQuorumThreshold &&
      factors.quorumCount <= factors.totalValidators;

    const signaturesValid =
      (!this.constitution.requireOperatorSignature || factors.operatorSignatureValid) &&
      (!this.constitution.requireSystemSignature || factors.systemSignatureValid);

    const root = (factors.zkRecursionRoot || "").trim();
    const rootValid = !!root && root !== "0x0" && root !== "0x" + "0".repeat(64);
    const zkValid = factors.zkStateConsistent && rootValid;

    const posturePassed = factors.postureScore <= this.constitution.maxAllowablePostureScore;
    const coherencePassed = factors.temporalCoherenceIndex >= this.constitution.minTemporalCoherence;

    const approved = quorumPassed && signaturesValid && zkValid && posturePassed && coherencePassed;

    let rejectionReason: string | undefined;
    if (!approved) {
      if (!quorumPassed) rejectionReason = "Quorum threshold not met";
      else if (!signaturesValid) rejectionReason = "Invalid operator or system signatures";
      else if (!zkValid) rejectionReason = "ZK proof state inconsistent or stale recursion root";
      else if (!posturePassed) rejectionReason = "Posture risk score exceeds safety thresholds";
      else if (!coherencePassed) rejectionReason = "Temporal coherence index below threshold";
    }

    const evaluatedFactors = [
      `Quorum: ${factors.quorumCount}/${factors.totalValidators} (min: ${this.constitution.minQuorumThreshold}) -> ${quorumPassed ? "PASS" : "FAIL"}`,
      `Signatures: ${signaturesValid ? "PASS" : "FAIL"}`,
      `ZK State: ${zkValid ? "PASS" : "FAIL"}`,
      `Posture: ${factors.postureScore} <= ${this.constitution.maxAllowablePostureScore} -> ${posturePassed ? "PASS" : "FAIL"}`,
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
