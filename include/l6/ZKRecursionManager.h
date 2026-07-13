#pragma once

#include "l6/ZKMetadata.h"
#include <string>
#include <stdexcept>
#include <optional>

namespace ailee::l6 {

class ZKRecursionManager {
public:
    // Builds the recursion bundle to be passed to the prover.
    // Validates continuity: previous_epoch_id + 1 == current_epoch_id
    // Validates state root continuity: previous end root == current start root.
    ZKRecursionBundle build_recursion_bundle(
        const ZKProofArtifact& previous_proof_artifact,
        const std::string& previous_state_root,
        uint64_t previous_epoch_id,
        const std::string& current_start_state_root,
        uint64_t current_epoch_id
    ) const;
};

} // namespace ailee::l6
