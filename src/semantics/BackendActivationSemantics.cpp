#include "semantics/BackendActivationSemantics.h"

namespace ailee::semantics {

bool BackendActivationSemantics::is_backend_allowed(EnvironmentType env, l6::ZKBackendType backend_type) {
    if (env == EnvironmentType::CI) {
        return backend_type == l6::ZKBackendType::MOCK;
    } else if (env == EnvironmentType::DEV) {
        return backend_type == l6::ZKBackendType::MOCK || backend_type == l6::ZKBackendType::HALO2_NATIVE;
    } else if (env == EnvironmentType::PROD) {
        return backend_type == l6::ZKBackendType::HALO2_NATIVE || backend_type == l6::ZKBackendType::PLONK_NATIVE;
    }
    return false;
}

} // namespace ailee::semantics
