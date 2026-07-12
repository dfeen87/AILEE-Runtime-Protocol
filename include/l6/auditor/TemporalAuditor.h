#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <cmath>
#include "simulation/validation/hice_contracts.h"

namespace ailee::l6::auditor {

struct TemporalEpochState {
    uint64_t epoch_id;
    simulation::validation::HiceMetrics metrics;
    std::array<uint8_t, 32> anchor_element;
    std::array<uint8_t, 32> replay_element;
};

class TemporalMetricsBuffer {
public:
    explicit TemporalMetricsBuffer(size_t window_size);

    void push_state(const TemporalEpochState& state);
    const std::vector<TemporalEpochState>& get_window() const;
    size_t get_window_size() const;
    bool is_full() const;

private:
    size_t window_size_;
    std::vector<TemporalEpochState> buffer_;
};

struct CoherenceScores {
    double average_covariance_error;
    double average_spectral_drift;
    double average_delta_memory;
    double average_context_leakage;
    double average_null_matching_rate;
    double average_delta_auc;

    double drift;
    double stability;
    double leakage;
    double covariance_evolution;
    double spectral_alignment;
};

struct AnchorCoherenceScores {
    double anchor_drift; // t vs t-1
    double replay_stability; // max(R_i - R_{i-1}) in window W
    double anchor_coherence;
};

struct ZkEpochValiditySurface {
    simulation::validation::HiceMetrics metrics;
    CoherenceScores coherence;
    AnchorCoherenceScores anchor;
};

class TemporalAuditor {
public:
    TemporalAuditor(double max_drift, double max_anchor_drift);

    ZkEpochValiditySurface evaluate(
        const TemporalMetricsBuffer& buffer,
        const TemporalEpochState& current_state
    ) const;

private:
    double max_drift_;
    double max_anchor_drift_;

    double compute_element_distance(const std::array<uint8_t, 32>& a, const std::array<uint8_t, 32>& b) const;
};

} // namespace ailee::l6::auditor
