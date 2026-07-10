#include "l6/DeterministicEpochAnchoring.h"

namespace ailee::l6 {

AnchorPlan compute_anchor_plan(uint64_t epoch_id, uint64_t anchor_interval) {
    if (anchor_interval == 0) {
        return AnchorPlan{
            AnchorDecision::SKIP,
            epoch_id,
            0 // Deterministic fallback when interval is 0
        };
    }

    uint64_t cycle_id = epoch_id / anchor_interval;
    AnchorDecision decision = (epoch_id > 0 && epoch_id % anchor_interval == 0)
                              ? AnchorDecision::ANCHOR
                              : AnchorDecision::SKIP;

    return AnchorPlan{
        decision,
        epoch_id,
        cycle_id
    };
}

} // namespace ailee::l6
