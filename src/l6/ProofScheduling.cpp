#include "l6/ProofScheduling.h"

namespace ailee::l6 {

ProofPlan compute_proof_plan(
    uint64_t epoch_id,
    const ProofSchedulingConfig& config
) {
    ProofPlan plan;
    plan.epoch_id = epoch_id;

    if (config.proof_interval_epochs == 0) {
        plan.decision = ProofDecision::SKIP;
        plan.proof_cycle_id = 0;
        return plan;
    }

    if (epoch_id % config.proof_interval_epochs == 0) {
        plan.decision = ProofDecision::ATTACH_PROOF;
    } else {
        plan.decision = ProofDecision::SKIP;
    }

    plan.proof_cycle_id = epoch_id / config.proof_interval_epochs;

    return plan;
}

} // namespace ailee::l6
