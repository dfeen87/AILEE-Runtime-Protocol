#include "l6/ZKRecursionManager.h"

namespace ailee::l6 {

ZKRecursionBundle ZKRecursionManager::build_recursion_bundle(
    const ZKProofArtifact& previous_proof_artifact,
    const std::string& previous_state_root,
    uint64_t previous_epoch_id,
    const std::string& current_start_state_root,
    uint64_t current_epoch_id
) const {
    if (previous_epoch_id + 1 != current_epoch_id) {
        throw std::invalid_argument("ZKRecursionManager: Epoch continuity violated.");
    }

    if (previous_state_root != current_start_state_root) {
        throw std::invalid_argument("ZKRecursionManager: State root continuity violated.");
    }

    ZKRecursionBundle bundle;
    bundle.previous_proof_artifact = previous_proof_artifact;
    bundle.previous_state_root = previous_state_root;
    bundle.previous_epoch_id = previous_epoch_id;

    return bundle;
}

} // namespace ailee::l6
