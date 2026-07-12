#include "l6/auditor/TemporalAuditor.h"

namespace ailee::l6::auditor {

TemporalMetricsBuffer::TemporalMetricsBuffer(size_t window_size)
    : window_size_(window_size) {}

void TemporalMetricsBuffer::push_state(const TemporalEpochState& state) {
    if (buffer_.size() >= window_size_) {
        buffer_.erase(buffer_.begin());
    }
    buffer_.push_back(state);
}

const std::vector<TemporalEpochState>& TemporalMetricsBuffer::get_window() const {
    return buffer_;
}

size_t TemporalMetricsBuffer::get_window_size() const {
    return window_size_;
}

bool TemporalMetricsBuffer::is_full() const {
    return buffer_.size() == window_size_;
}

TemporalAuditor::TemporalAuditor(double max_drift, double max_anchor_drift)
    : max_drift_(max_drift), max_anchor_drift_(max_anchor_drift) {}

double TemporalAuditor::compute_element_distance(const std::array<uint8_t, 32>& a, const std::array<uint8_t, 32>& b) const {
    double sum_sq = 0.0;
    for (size_t i = 0; i < 32; ++i) {
        double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sum_sq += diff * diff;
    }
    return std::sqrt(sum_sq);
}

ZkEpochValiditySurface TemporalAuditor::evaluate(
    const TemporalMetricsBuffer& buffer,
    const TemporalEpochState& current_state
) const {
    ZkEpochValiditySurface surface;
    surface.metrics = current_state.metrics;

    // Default initialization for when buffer is empty
    surface.coherence = {0,0,0,0,0,0, 0,1,0,0,0};
    surface.anchor = {0,0,1};

    auto window = buffer.get_window();
    if (window.empty()) {
        return surface;
    }

    // Include current_state in the logical window computation
    std::vector<TemporalEpochState> full_window = window;
    full_window.push_back(current_state);

    size_t n = full_window.size();

    // 1. Coherence(m, W) - average over window
    double sum_cov = 0, sum_spec = 0, sum_mem = 0, sum_leak = 0, sum_null = 0, sum_auc = 0;
    for (const auto& state : full_window) {
        sum_cov += state.metrics.covariance_error;
        sum_spec += state.metrics.spectral_drift;
        sum_mem += state.metrics.delta_memory;
        sum_leak += state.metrics.context_leakage;
        sum_null += state.metrics.null_matching_rate;
        sum_auc += state.metrics.delta_auc;
    }

    surface.coherence.average_covariance_error = sum_cov / n;
    surface.coherence.average_spectral_drift = sum_spec / n;
    surface.coherence.average_delta_memory = sum_mem / n;
    surface.coherence.average_context_leakage = sum_leak / n;
    surface.coherence.average_null_matching_rate = sum_null / n;
    surface.coherence.average_delta_auc = sum_auc / n;

    // 2. Drift(W) - sum of absolute differences for some key metrics (e.g. covariance, spectral)
    double total_drift = 0.0;
    for (size_t i = 1; i < n; ++i) {
        total_drift += std::abs(full_window[i].metrics.covariance_error - full_window[i-1].metrics.covariance_error);
        total_drift += std::abs(full_window[i].metrics.spectral_drift - full_window[i-1].metrics.spectral_drift);
    }
    surface.coherence.drift = total_drift;

    // 3. Stability(W)
    double stab = 1.0 - (total_drift / (max_drift_ > 0 ? max_drift_ : 1.0));
    surface.coherence.stability = std::max(0.0, stab);

    // 4. Leakage(W) - max context leakage
    double max_leak = 0.0;
    for (const auto& state : full_window) {
        if (state.metrics.context_leakage > max_leak) {
            max_leak = state.metrics.context_leakage;
        }
    }
    surface.coherence.leakage = max_leak;

    // 5. Covariance evolution ΔΣ(W)
    surface.coherence.covariance_evolution =
        full_window.back().metrics.covariance_error - full_window.front().metrics.covariance_error;

    // 6. Spectral alignment Δλ(W)
    surface.coherence.spectral_alignment =
        std::abs(full_window.back().metrics.spectral_drift - full_window.front().metrics.spectral_drift);

    // Anchor Validation
    // anchor_drift(t) = || a_t - a_{t-1} ||_2
    const auto& prev_state = window.back();
    surface.anchor.anchor_drift = compute_element_distance(current_state.anchor_element, prev_state.anchor_element);

    // replay_stability(W) = max || R_i - R_{i-1} ||_2
    double max_replay_drift = compute_element_distance(current_state.replay_element, prev_state.replay_element);
    for (size_t i = 1; i < window.size(); ++i) {
        double dist = compute_element_distance(window[i].replay_element, window[i-1].replay_element);
        if (dist > max_replay_drift) {
            max_replay_drift = dist;
        }
    }
    surface.anchor.replay_stability = max_replay_drift;

    // anchor_coherence(W)
    double sum_anchor_drift = surface.anchor.anchor_drift;
    for (size_t i = 1; i < window.size(); ++i) {
        sum_anchor_drift += compute_element_distance(window[i].anchor_element, window[i-1].anchor_element);
    }

    double anchor_coh = 1.0 - (sum_anchor_drift / (max_anchor_drift_ > 0 ? max_anchor_drift_ : 1.0));
    surface.anchor.anchor_coherence = std::max(0.0, anchor_coh);

    return surface;
}

} // namespace ailee::l6::auditor
