#pragma once

#include <cstdint>

namespace ailee::l6::isla {

enum class AnchorCadence {
    NORMAL,   // Default cadence: anchor every epoch
    TIGHT,    // Higher frequency: anchor more aggressively
    RELAXED   // Lower frequency: anchor less frequently
};

struct IslaTuningDecision {
    uint32_t new_batch_size;
    uint32_t new_proof_interval_ms;
    uint32_t new_worker_allocation;
    AnchorCadence new_anchor_cadence;
};

} // namespace ailee::l6::isla
