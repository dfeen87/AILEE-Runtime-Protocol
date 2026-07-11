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
};

struct PolicyState {
    uint32_t anchor_frequency;
    uint32_t proof_frequency;
    uint32_t max_replay_epochs;
    double min_coherence_score;
    uint32_t epoch_length;
    EnvironmentType environment_type;
    uint32_t metadata_version;
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
