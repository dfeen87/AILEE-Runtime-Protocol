#include "l6/AnchorZKMetadata.h"

namespace ailee::l6 {

AnchorPayload attach_zk_to_anchor(
    const AnchorPayload& base_payload,
    const ProofPlan& proof_plan,
    const ZKProofStub* proof_stub
) {
    AnchorPayload new_payload;
    new_payload.epoch_id = base_payload.epoch_id;
    new_payload.state_root_hash = base_payload.state_root_hash;

    if (proof_plan.decision == ProofDecision::ATTACH_PROOF && proof_stub != nullptr) {
        new_payload.zk_metadata.constraint_set_id = proof_stub->constraint_set_id;
        new_payload.zk_metadata.transcript_id = proof_stub->transcript_id;
        new_payload.zk_metadata.proof_id = proof_stub->proof_id;
    } else {
        new_payload.zk_metadata.constraint_set_id = "";
        new_payload.zk_metadata.transcript_id = "";
        new_payload.zk_metadata.proof_id = "";
    }

    return new_payload;
}

} // namespace ailee::l6
