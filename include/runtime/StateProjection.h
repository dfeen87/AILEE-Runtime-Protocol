#pragma once

#include "l6/ExternalSchema.h"

namespace ailee {
namespace runtime {

class StateProjection {
public:
    static l6::ExternalStateSnapshot generateSnapshot();
};

} // namespace runtime
} // namespace ailee
