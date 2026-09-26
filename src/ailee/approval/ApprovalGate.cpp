#include "ailee/approval/ApprovalGate.hpp"
#include <algorithm>

namespace ailee::approval {

GateDecision ApprovalGate::evaluate_factors(const ApprovalFactors& factors) const {
    const auto& rules = constitution_.rules();
    GateDecision decision;

    // Factor 1: Threshold quorum
    bool quorum_count_ok = (factors.quorum_count >= rules.min_quorum_threshold);
    bool total_validators_ok = (factors.total_validators >= rules.min_quorum_threshold);
    bool bounds_ok = (factors.quorum_count <= factors.total_validators);
    decision.quorum_passed = quorum_count_ok && total_validators_ok && bounds_ok;

    decision.evaluated_factors.push_back(
        "Factor 1 Quorum: " + std::to_string(factors.quorum_count) + "/" +
        std::to_string(factors.total_validators) + " (min threshold: " +
        std::to_string(rules.min_quorum_threshold) + ") -> " +
        (decision.quorum_passed ? "PASS" : "FAIL")
    );

    // Factor 2: Cryptographic signatures
    bool op_ok = (!rules.require_operator_signature || factors.operator_signature_valid);
    bool sys_ok = (!rules.require_system_signature || factors.system_signature_valid);
    decision.signatures_valid = op_ok && sys_ok;
    decision.evaluated_factors.push_back(
        "Factor 2 Signatures: operator=" + std::string(op_ok ? "OK" : "INVALID") +
        ", system=" + std::string(sys_ok ? "OK" : "INVALID") + " -> " +
        (decision.signatures_valid ? "PASS" : "FAIL")
    );

    // Factor 3: ZK Proof state & Recursion Root consistency
    bool root_non_empty = !factors.zk_recursion_root.empty();
    bool root_non_zero = (factors.zk_recursion_root != "0x0000000000000000000000000000000000000000000000000000000000000000" &&
                          factors.zk_recursion_root != "0x0");
    decision.zk_valid = factors.zk_state_consistent && root_non_empty && root_non_zero;

    decision.evaluated_factors.push_back(
        "Factor 3 ZK State: " + std::string(decision.zk_valid ? "CONSISTENT" : "INCONSISTENT") +
        " (root: " + (root_non_empty ? (root_non_zero ? "VALID" : "ZERO_STALE") : "EMPTY") + ")"
    );

    // Factor 4: Posture score threshold enforcement (NO BYPASS VECTOR)
    double max_allowed = rules.max_allowable_posture_score;
    decision.posture_passed = (factors.posture_score <= max_allowed);
    decision.evaluated_factors.push_back(
        "Factor 4 Posture Score: " + std::to_string(factors.posture_score) +
        " <= " + std::to_string(max_allowed) + " -> " +
        (decision.posture_passed ? "PASS" : "FAIL")
    );

    // Factor 5: Temporal coherence index
    decision.coherence_passed = (factors.temporal_coherence_index >= rules.min_temporal_coherence);
    decision.evaluated_factors.push_back(
        "Factor 5 Temporal Coherence: " + std::to_string(factors.temporal_coherence_index) +
        " >= " + std::to_string(rules.min_temporal_coherence) + " -> " +
        (decision.coherence_passed ? "PASS" : "FAIL")
    );

    // Final Approval Logic
    decision.approved = decision.quorum_passed &&
                        decision.signatures_valid &&
                        decision.zk_valid &&
                        decision.posture_passed &&
                        decision.coherence_passed;

    if (!decision.approved) {
        if (!decision.quorum_passed) decision.rejection_reason = "Quorum threshold not met";
        else if (!decision.signatures_valid) decision.rejection_reason = "Invalid operator or system signatures";
        else if (!decision.zk_valid) decision.rejection_reason = "ZK proof state inconsistent or stale recursion root";
        else if (!decision.posture_passed) decision.rejection_reason = "Posture risk score exceeds safety thresholds";
        else if (!decision.coherence_passed) decision.rejection_reason = "Temporal coherence index below threshold";
    }

    return decision;
}

} // namespace ailee::approval
