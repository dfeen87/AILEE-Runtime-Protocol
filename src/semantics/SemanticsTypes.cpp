#include "semantics/SemanticsTypes.h"

namespace ailee::semantics {

PolicyState load_policy(const Config& config) {
    PolicyState state;
    state.anchor_frequency = config.anchor_frequency;
    state.proof_frequency = config.proof_frequency;
    state.max_replay_epochs = config.max_replay_epochs;
    state.min_coherence_score = config.min_coherence_score;
    state.epoch_length = config.epoch_length;
    state.environment_type = config.environment_type;
    state.metadata_version = config.metadata_version;
    return state;
}

} // namespace ailee::semantics
