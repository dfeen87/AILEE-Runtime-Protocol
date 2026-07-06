#pragma once

#include "ambient_ai_runtime_model.hpp"
#include "../../wnn/core/wave_native_network_core.hpp"
#include <cstdint>
#include <vector>

namespace ailee {
namespace core {
namespace runtime {

class WnnRuntimeSchedulerIntegration {
public:
    // WaveState -> AmbientRuntimeScheduler
    bool schedule_wave_state(const ailee::wnn::WaveState& state, ailee::runtime::AmbientRuntimeScheduler& scheduler, uint64_t logicalTimestamp) const;
};

} // namespace runtime
} // namespace core
} // namespace ailee
