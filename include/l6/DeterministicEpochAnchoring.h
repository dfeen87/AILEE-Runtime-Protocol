#pragma once

#include <cstdint>

namespace ailee::l6 {

enum class AnchorDecision {
    SKIP,
    ANCHOR
};

struct AnchorPlan {
    AnchorDecision decision;
    uint64_t epoch_id;
    uint64_t anchor_cycle_id;
};

// Pure deterministic function to compute if an epoch should be anchored
// and what its cycle id is.
AnchorPlan compute_anchor_plan(uint64_t epoch_id, uint64_t anchor_interval);

} // namespace ailee::l6
