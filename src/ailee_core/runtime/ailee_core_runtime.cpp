#include "ailee_core_runtime.hpp"
#include <vector>
#include <cstring>

namespace ailee {
namespace core {
namespace runtime {

bool WnnRuntimeSchedulerIntegration::schedule_wave_state(const ailee::wnn::WaveState& state, ailee::runtime::AmbientRuntimeScheduler& scheduler, uint64_t logicalTimestamp) const {
    std::vector<uint8_t> payload(sizeof(ailee::wnn::WaveState));
    std::memcpy(payload.data(), &state, sizeof(ailee::wnn::WaveState));

    if (!scheduler.validateTaskConstraints(payload)) {
        return false;
    }
    scheduler.executeScheduledTasks(logicalTimestamp);
    return true;
}

} // namespace runtime
} // namespace core
} // namespace ailee
