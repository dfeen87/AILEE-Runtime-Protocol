#include "DeterministicEpochExecutorWrapper.hpp"
#include "../../include/ambient_ai_runtime_model.hpp"

namespace ailee {
namespace heartbeat {

DeterministicEpochExecutorWrapper::DeterministicEpochExecutorWrapper(ailee::runtime::AmbientRuntime& runtime)
    : runtime_(runtime) {}

std::string DeterministicEpochExecutorWrapper::run_epoch(uint64_t epoch_index) {
    auto result = runtime_.run_epoch(epoch_index);
    return result.state_root;
}

} // namespace heartbeat
} // namespace ailee
