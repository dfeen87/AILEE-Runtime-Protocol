#include "l6/erl/TemporalEnergyCoordinator.h"

namespace ailee::l6::erl {

TemporalEnergyCoordinator::TemporalEnergyCoordinator(const semantics::Config& config)
    : config_(config), last_energy_(0.0), has_last_energy_(false) {}

EnergyAdaptationDecision TemporalEnergyCoordinator::evaluate(
    const simulation::validation::HiceMetrics& current_metrics,
    const auditor::ZkEpochValiditySurface& validity_surface
) {
    EnergyAdaptationDecision decision;

    // 1. Calculate E(t)
    double current_energy =
        config_.alpha_drift * validity_surface.coherence.drift +
        config_.alpha_spectral * current_metrics.spectral_drift +
        config_.alpha_memory * current_metrics.delta_memory;

    // 2. Calculate energy_drift
    double energy_drift = 0.0;
    if (has_last_energy_) {
        energy_drift = current_energy - last_energy_;
    }

    // 3. Calculate energy_stability
    double energy_stability = 1.0 - (std::abs(energy_drift) / (config_.max_energy_drift > 0.0 ? config_.max_energy_drift : 1.0));
    energy_stability = std::clamp(energy_stability, 0.0, 1.0);

    // 4. Calculate energy_cost_of_drift
    double energy_cost_of_drift = config_.base_cost_factor * std::abs(energy_drift);

    // 5. Calculate energy_anchor_coherence
    double total_drift_magnitude = std::abs(energy_drift) + std::abs(validity_surface.anchor.anchor_drift);
    double energy_anchor_coherence = 1.0 - (total_drift_magnitude / (config_.max_energy_anchor_drift > 0.0 ? config_.max_energy_anchor_drift : 1.0));
    energy_anchor_coherence = std::clamp(energy_anchor_coherence, 0.0, 1.0);

    // 6. Calculate energy_predictive_score
    // Using leakage from temporal auditor's window evaluation
    double temporal_leakage = validity_surface.coherence.leakage;
    double energy_predictive_score = std::abs(energy_drift) * temporal_leakage;

    // Build the resilience surface
    decision.surface.energy_drift = energy_drift;
    decision.surface.energy_stability = energy_stability;
    decision.surface.energy_cost_of_drift = energy_cost_of_drift;
    decision.surface.energy_anchor_coherence = energy_anchor_coherence;
    decision.surface.energy_predictive_score = energy_predictive_score;

    // 7. Evaluate Behavioral Flags
    decision.should_reduce_heavy_ops = (energy_predictive_score > config_.energy_predictive_threshold);
    decision.should_delay_replication = (energy_stability < config_.energy_stability_margin);
    decision.should_increase_backoff = (std::abs(energy_drift) > config_.max_energy_drift);

    // Update state for next epoch
    last_energy_ = current_energy;
    has_last_energy_ = true;

    return decision;
}

} // namespace ailee::l6::erl
