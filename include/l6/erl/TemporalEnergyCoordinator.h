#pragma once

#include "semantics/SemanticsTypes.h"
#include "semantics/MetadataSemantics.h"
#include "l6/auditor/TemporalAuditor.h"
#include "simulation/validation/hice_contracts.h"
#include <cmath>
#include <algorithm>

namespace ailee::l6::erl {

struct EnergyAdaptationDecision {
    semantics::EnergyResilienceSurface surface;
    bool should_reduce_heavy_ops;
    bool should_delay_replication;
    bool should_increase_backoff;
};

class TemporalEnergyCoordinator {
public:
    explicit TemporalEnergyCoordinator(const semantics::Config& config);

    EnergyAdaptationDecision evaluate(
        const simulation::validation::HiceMetrics& current_metrics,
        const auditor::ZkEpochValiditySurface& validity_surface
    );

    double get_last_energy() const { return last_energy_; }

private:
    semantics::Config config_;
    double last_energy_;
    bool has_last_energy_;
};

} // namespace ailee::l6::erl
