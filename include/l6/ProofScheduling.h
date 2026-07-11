#pragma once

#include <cstdint>
#include "l6/DeterministicEpochAnchoring.h"

namespace ailee::l6 {


struct ProofSchedulingConfig {
    uint64_t proof_interval_epochs;   // e.g., generate proof every N epochs
    uint64_t max_proof_window_size;   // max number of epochs per proof window
};

enum class ProofDecision {
    SKIP,
    ATTACH_PROOF
};

struct SchedulerDecision {
    AnchorDecision anchor_decision;
    ProofDecision proof_decision;
};

class EpochScheduler {
public:
    virtual ~EpochScheduler() = default;
    virtual SchedulerDecision get_decision(uint64_t epoch_id) const = 0;
};


struct ProofPlan {
    ProofDecision decision;
    uint64_t epoch_id;
    uint64_t proof_cycle_id;   // deterministic cycle index
};

ProofPlan compute_proof_plan(
    uint64_t epoch_id,
    const ProofSchedulingConfig& config
);

} // namespace ailee::l6
