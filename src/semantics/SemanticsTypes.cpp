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

    state.alpha_drift = config.alpha_drift;
    state.alpha_spectral = config.alpha_spectral;
    state.alpha_memory = config.alpha_memory;
    state.max_energy_drift = config.max_energy_drift;
    state.base_cost_factor = config.base_cost_factor;
    state.max_energy_anchor_drift = config.max_energy_anchor_drift;
    state.energy_predictive_threshold = config.energy_predictive_threshold;
    state.energy_stability_margin = config.energy_stability_margin;

    return state;
}

} // namespace ailee::semantics
