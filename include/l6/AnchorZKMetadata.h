#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include "l6/ZKMetadata.h"
#include "l6/ZKDeterministicValidator.h"
#include "l6/ProofScheduling.h"

namespace ailee::l6 {

struct ZKAnchorMetadata {
    std::string proof_id;
    std::string constraint_set_id;
    std::string transcript_id;
    DeterministicZKStatus validation_status;

    std::vector<uint8_t> to_bytes() const;
};

struct AnchorPayload {
    uint64_t epoch_id;
    std::string state_root_hash;
    ZKAnchorMetadata zk_metadata;

    std::vector<uint8_t> to_bytes() const;
};

// Removed attach_zk_to_anchor because it's replaced by orchestrate_epoch logic
// Let's keep it if we need, but maybe it's cleaner to remove and just update tests.
// Actually, I'll update it to use orchestrate_epoch logic or just keep the updated version.

AnchorPayload attach_zk_to_anchor(
    const AnchorPayload& base_payload,
    const ProofPlan& proof_plan,
    const ZKProofMetadata* proof_metadata,
    const ZKConstraintSet* constraints = nullptr,
    const ZKTranscript* transcript = nullptr
);

} // namespace ailee::l6
