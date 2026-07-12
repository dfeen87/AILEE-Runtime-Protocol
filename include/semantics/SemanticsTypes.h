#pragma once

#include <cstdint>
#include <string>

namespace ailee::semantics {

enum class EnvironmentType {
    CI,
    DEV,
    PROD
};

struct Config {
    uint32_t anchor_frequency = 1;
    uint32_t proof_frequency = 1;
    uint32_t max_replay_epochs = 100;
    double min_coherence_score = 0.5;
    uint32_t epoch_length = 1000;
    EnvironmentType environment_type = EnvironmentType::DEV;
    uint32_t metadata_version = 1;

    // ERL parameters
    double alpha_drift = 1.0;
    double alpha_spectral = 1.0;
    double alpha_memory = 1.0;
    double max_energy_drift = 10.0;
    double base_cost_factor = 1.0;
    double max_energy_anchor_drift = 20.0;
    double energy_predictive_threshold = 5.0;
    double energy_stability_margin = 0.5;
};

struct PolicyState {
    uint32_t anchor_frequency;
    uint32_t proof_frequency;
    uint32_t max_replay_epochs;
    double min_coherence_score;
    uint32_t epoch_length;
    EnvironmentType environment_type;
    uint32_t metadata_version;

    // ERL parameters
    double alpha_drift;
    double alpha_spectral;
    double alpha_memory;
    double max_energy_drift;
    double base_cost_factor;
    double max_energy_anchor_drift;
    double energy_predictive_threshold;
    double energy_stability_margin;
};

PolicyState load_policy(const Config& config);

struct MeshState {
    uint32_t node_count = 0;
    uint32_t active_links = 0;
    double latency_mean = 0.0;
    double latency_variance = 0.0;
    uint32_t partition_count = 0;
    uint32_t recent_failures = 0;
};

} // namespace ailee::semantics
