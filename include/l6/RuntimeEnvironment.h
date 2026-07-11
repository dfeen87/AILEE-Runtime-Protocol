#pragma once

#include "l6/ZKProvingBackend.h"
#include "semantics/SemanticsTypes.h"

namespace ailee::l6 {

struct RuntimeEnvironment {
    bool is_ci = false;
    bool is_dev = false;
    bool is_staging = false;
    bool is_mainnet_sim = false;

    ailee::semantics::EnvironmentType get_environment_type() const {
        if (is_ci) return ailee::semantics::EnvironmentType::CI;
        // Assume DEV unless explicitly something else, could check a prod flag if added later
        return ailee::semantics::EnvironmentType::DEV;
    }
};

ZKBackendType select_backend_type(
    const RuntimeEnvironment& env,
    const ZKBackendConfig& config
);

} // namespace ailee::l6
