#include "l6/iao/IntelligenceAssistedOrchestrator.h"
#include <cmath>

namespace ailee::l6::iao {

IntelligenceAssistedOrchestrator::IntelligenceAssistedOrchestrator() {}

OrchestrationGuidanceReport IntelligenceAssistedOrchestrator::evaluate(
    const auditor::TemporalMetricsBuffer& buffer,
    const auditor::ZkEpochValiditySurface& validity_surface,
    const erl::EnergyAdaptationDecision& energy_decision
) const {
    OrchestrationGuidanceReport report;

    // 1. Long-Term Stability Score
    // \text{long\_term\_stability}(W) = 1.0 - \frac{\sum_{i=t-n}^{t} |\text{drift}(i)|}{\text{max\_drift\_window}}
    // We calculate sum over window: here the drift for state i is the change from state i-1
    double sum_drift = 0.0;
    const auto& window = buffer.get_window();

    // Using covariance error drift as the canonical 'drift(i)' based on earlier auditor logic
    // Wait, the specification says:
    // "drift_accumulation_forecast = \beta_1 * drift(t) + \beta_2 * drift(t-1)"
    // and
    // "long_term_stability(W) = 1.0 - (\sum_{i=t-n}^{t} |drift(i)|) / max_drift_window"
    // Let's get the sequence of drifts.

    std::vector<double> drifts;
    if (window.size() > 1) {
        for (size_t i = 1; i < window.size(); ++i) {
            // sum of abs diffs in key metrics: let's use the same as TemporalAuditor.cpp:
            double d = std::abs(window[i].metrics.covariance_error - window[i-1].metrics.covariance_error) +
                       std::abs(window[i].metrics.spectral_drift - window[i-1].metrics.spectral_drift);
            drifts.push_back(d);
            sum_drift += d;
        }
    }

    report.long_term_stability_score = 1.0 - (sum_drift / MAX_DRIFT_WINDOW);

    // 2. Drift Accumulation Forecast
    double drift_t = 0.0;
    double drift_t_minus_1 = 0.0;
    if (drifts.size() >= 1) drift_t = drifts.back();
    if (drifts.size() >= 2) drift_t_minus_1 = drifts[drifts.size()-2];

    report.drift_accumulation_forecast = (BETA_1 * drift_t) + (BETA_2 * drift_t_minus_1);

    // 3. Predicted Energy Trend
    report.predicted_energy_trend = GAMMA * energy_decision.surface.energy_predictive_score;

    // 4. Anchor-Chain Risk
    // anchor_risk(t) = |anchor_drift(t)| / max_anchor_drift
    report.anchor_chain_risk = std::abs(validity_surface.anchor.anchor_drift) / MAX_ANCHOR_DRIFT;

    // 5. Temporal Leakage Projection
    // context_leakage is in validity_surface.metrics.context_leakage (but we use the most recent from window if buffer isn't empty, wait, validity_surface.metrics has it)
    report.temporal_leakage_projection = DELTA * validity_surface.metrics.context_leakage;

    // Behavioral Recommendations

    if (report.long_term_stability_score < 0.5) {
        report.recommended_scheduling_mode = "CONSERVATIVE_SCHEDULING";
    } else {
        report.recommended_scheduling_mode = "AGGRESSIVE_SCHEDULING";
    }

    if (report.anchor_chain_risk > 0.7) {
        report.recommended_replication_mode = "HIGH_REPLICATION";
    } else {
        report.recommended_replication_mode = "STANDARD_REPLICATION";
    }

    if (report.predicted_energy_trend < 0) {
        report.recommended_backoff_strategy = "INCREASE_BACKOFF";
    } else {
        report.recommended_backoff_strategy = "MAINTAIN_BACKOFF";
    }

    return report;
}

} // namespace ailee::l6::iao
