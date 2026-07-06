#ifndef AILEE_HEARTBEAT_DETERMINISTIC_EPOCH_EXECUTOR_WRAPPER_HPP
#define AILEE_HEARTBEAT_DETERMINISTIC_EPOCH_EXECUTOR_WRAPPER_HPP

#include <cstdint>
#include <string>

// Forward declaration of the existing runtime
namespace ailee {
namespace runtime {
    class AmbientRuntime;
}
}

namespace ailee {
namespace heartbeat {

class DeterministicEpochExecutorWrapper {
public:
    explicit DeterministicEpochExecutorWrapper(ailee::runtime::AmbientRuntime& runtime);

    // Wraps the call to the deterministic runtime
    std::string run_epoch(uint64_t epoch_index);

private:
    ailee::runtime::AmbientRuntime& runtime_;
};

} // namespace heartbeat
} // namespace ailee

#endif // AILEE_HEARTBEAT_DETERMINISTIC_EPOCH_EXECUTOR_WRAPPER_HPP
