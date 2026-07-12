#pragma once

#include "l6/ZKProvingBackend.h"
#include "semantics/SemanticsTypes.h"
#include "simulation/validation/hice_contracts.h"

namespace ailee::l6 {

struct RuntimeEnvironment {
    bool is_ci = false;
    bool is_dev = false;
    bool is_staging = false;
    bool is_mainnet_sim = false;

    ailee::simulation::validation::HiceThresholds hice_thresholds = ailee::simulation::validation::HiceThresholds::v27_defaults();
    double practical_margin = 0.03;

    ailee::semantics::EnvironmentType get_environment_type() const {
        if (is_ci) return ailee::semantics::EnvironmentType::CI;
        // Assume DEV unless explicitly something else, could check a prod flag if added later
        return ailee::semantics::EnvironmentType::DEV;
    }

    const ailee::simulation::validation::HiceThresholds& get_hice_thresholds() const {
        return hice_thresholds;
    }

    double get_practical_margin() const {
        return practical_margin;
    }
};

ZKBackendType select_backend_type(
    const RuntimeEnvironment& env,
    const ZKBackendConfig& config
);

} // namespace ailee::l6
