#pragma once

#include "semantics/SemanticsTypes.h"
#include "l6/ZKProvingBackend.h"

namespace ailee::semantics {

struct BackendActivationSemantics {
    static bool is_backend_allowed(EnvironmentType env, l6::ZKBackendType backend_type);
};

} // namespace ailee::semantics
