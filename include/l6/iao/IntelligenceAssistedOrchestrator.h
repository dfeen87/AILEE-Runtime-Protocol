#pragma once

#include "l6/auditor/TemporalAuditor.h"
#include "l6/erl/TemporalEnergyCoordinator.h"
#include <string>
#include <cmath>

namespace ailee::l6::iao {

struct OrchestrationGuidanceReport {
    double long_term_stability_score;
    double drift_accumulation_forecast;
    double predicted_energy_trend;
    double anchor_chain_risk;
    double temporal_leakage_projection;

    std::string recommended_scheduling_mode;
    std::string recommended_replication_mode;
    std::string recommended_backoff_strategy;
};

class IntelligenceAssistedOrchestrator {
public:
    IntelligenceAssistedOrchestrator();

    OrchestrationGuidanceReport evaluate(
        const auditor::TemporalMetricsBuffer& buffer,
        const auditor::ZkEpochValiditySurface& validity_surface,
        const erl::EnergyAdaptationDecision& energy_decision
    ) const;

private:
    static constexpr double MAX_DRIFT_WINDOW = 32.0;
    static constexpr double BETA_1 = 0.65;
    static constexpr double BETA_2 = 0.35;
    static constexpr double GAMMA = 1.0;
    static constexpr double DELTA = 1.0;
    static constexpr double MAX_ANCHOR_DRIFT = 1000.0;
};

} // namespace ailee::l6::iao
