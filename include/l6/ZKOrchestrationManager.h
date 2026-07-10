#pragma once

#include <string>
#include <cstdint>
#include "l6/DeterministicEpochAnchoring.h"
#include "l6/ProofScheduling.h"
#include "l6/ParameterGovernor.h"
#include "l6/ZKMetadata.h"
#include "l6/AnchorZKMetadata.h"
#include "l6/ZKDeterministicValidator.h"

namespace ailee::l6 {

struct OrchestrationContext {
    uint64_t epoch_id;
    AnchorPlan anchor_plan;
    ProofPlan proof_plan;
    ParameterAdjustments params;
};

struct OrchestrationResult {
    AnchorPayload anchor_payload;
    bool should_anchor;
    bool should_attach_proof;
};

OrchestrationResult orchestrate_epoch(
    const OrchestrationContext& ctx,
    const ZKProofMetadata* proof_metadata,
    const ZKConstraintSet* constraints,
    const ZKTranscript* transcript,
    const std::string& state_root_hash
);

} // namespace ailee::l6
