#include "l6/ZKOrchestrationManager.h"

namespace ailee::l6 {

OrchestrationResult orchestrate_epoch(
    const OrchestrationContext& ctx,
    const ZKProofStub* proof_stub,
    const std::string& state_root_hash
) {
    OrchestrationResult result;

    // 1. Determine anchoring
    result.should_anchor = (ctx.anchor_plan.decision == AnchorDecision::ANCHOR);

    // 2. Determine proof attachment
    result.should_attach_proof = (ctx.proof_plan.decision == ProofDecision::ATTACH_PROOF) && (proof_stub != nullptr);

    // 3. Construct ZK metadata
    ZKAnchorMetadata zk_metadata;
    if (result.should_attach_proof) {
        zk_metadata.constraint_set_id = proof_stub->constraint_set_id;
        zk_metadata.transcript_id = proof_stub->transcript_id;
        zk_metadata.proof_id = proof_stub->proof_id;
    } else {
        zk_metadata.constraint_set_id = "";
        zk_metadata.transcript_id = "";
        zk_metadata.proof_id = "";
    }

    // 4. Construct anchor payload
    result.anchor_payload.epoch_id = ctx.epoch_id;
    result.anchor_payload.state_root_hash = state_root_hash;
    result.anchor_payload.zk_metadata = zk_metadata;

    // 5. Return
    return result;
}

} // namespace ailee::l6
