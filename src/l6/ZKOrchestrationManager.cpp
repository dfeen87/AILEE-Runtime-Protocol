#include "l6/ZKOrchestrationManager.h"

namespace ailee::l6 {

OrchestrationResult orchestrate_epoch(
    const OrchestrationContext& ctx,
    const ZKProofMetadata* proof_metadata,
    const ZKConstraintSet* constraints,
    const ZKTranscript* transcript,
    const std::string& state_root_hash
) {
    OrchestrationResult result;

    // 1. Determine anchoring
    bool should_anchor = (ctx.anchor_plan.decision == AnchorDecision::ANCHOR);

    // 2. Determine proof attachment
    bool scheduling_says_attach = (ctx.proof_plan.decision == ProofDecision::ATTACH_PROOF);

    ZKAnchorMetadata zk_metadata;

    if (scheduling_says_attach && proof_metadata != nullptr) {
        if (constraints != nullptr && transcript != nullptr) {
            auto check = validate_zk_metadata(*proof_metadata, *constraints, *transcript);
            if (check.status == DeterministicZKStatus::OK) {
                zk_metadata.constraint_set_id = proof_metadata->constraint_set_id;
                zk_metadata.transcript_id = proof_metadata->transcript_id;
                zk_metadata.proof_id = proof_metadata->proof_id;
                zk_metadata.validation_status = DeterministicZKStatus::OK;
            } else {
                zk_metadata.constraint_set_id = "";
                zk_metadata.transcript_id = "";
                zk_metadata.proof_id = "";
                zk_metadata.validation_status = check.status;
            }
        } else {
            zk_metadata.constraint_set_id = "";
            zk_metadata.transcript_id = "";
            zk_metadata.proof_id = "";
            zk_metadata.validation_status = (constraints == nullptr) ? DeterministicZKStatus::EMPTY_CONSTRAINTS : DeterministicZKStatus::EMPTY_TRANSCRIPT;
        }
    } else {
        zk_metadata.constraint_set_id = "";
        zk_metadata.transcript_id = "";
        zk_metadata.proof_id = "";
        zk_metadata.validation_status = DeterministicZKStatus::OK;
    }

    // 4. Construct anchor payload
    AnchorPayload payload;
    payload.epoch_id = ctx.epoch_id;
    payload.state_root_hash = state_root_hash;
    payload.zk_metadata = zk_metadata;

    result.anchor_payload = payload;
    result.should_anchor = should_anchor;
    result.should_attach_proof = scheduling_says_attach;

    return result;
}

} // namespace ailee::l6
