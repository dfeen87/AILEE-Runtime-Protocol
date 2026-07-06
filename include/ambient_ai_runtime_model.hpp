#ifndef AMBIENT_AI_RUNTIME_MODEL_HPP
#define AMBIENT_AI_RUNTIME_MODEL_HPP

#include <memory>
#include <cstdint>
#include <vector>
#include <string>

// Forward declarations
namespace ailee {
namespace runtime {
    class AmbientRuntimeScheduler;
    class AmbientRuntimeEventLoop;
    struct AmbientRuntimeConfig {};
    class AmbientRuntimeStateMachine;
    class AmbientSubsystemContext;
}
}

namespace ailee {
namespace runtime {

class AmbientRuntimeEventLoop {
public:
    // Polls deterministic events based on L2 logical ticks
    // Strictly constrained to avoid tight-loops and excessive CPU usage
    void tick(uint64_t logicalTimestamp);

    // Enqueues deterministic tasks for the runtime scheduler
    bool enqueueTask(const std::vector<uint8_t>& serializedTask);
};

class AmbientRuntimeScheduler {
public:
    // Schedules execution based purely on deterministic ticks and logical time
    void executeScheduledTasks(uint64_t logicalTimestamp);

    // Rejects tasks that violate low-power bounds or L2 consensus timing
    bool validateTaskConstraints(const std::vector<uint8_t>& serializedTask) const;
};

class AmbientRuntime {
public:
    struct EpochResult {
        std::string state_root;
    };

    explicit AmbientRuntime(const AmbientRuntimeConfig& config);
    ~AmbientRuntime();

    // Initiates the deterministic runtime loop
    bool initialize();

    // Steps the entire runtime deterministically based on L1 block heights and L2 ticks
    void step(uint64_t logicalTimestamp, uint64_t currentBitcoinHeight);

    // Executes a full deterministic epoch using existing L2 Merkleizer and Anchoring Pipeline
    EpochResult run_epoch(uint64_t epoch_index);

    // Graceful teardown
    void shutdown();

private:
    std::unique_ptr<AmbientRuntimeScheduler> scheduler;
    std::unique_ptr<AmbientRuntimeEventLoop> eventLoop;
    std::unique_ptr<AmbientRuntimeStateMachine> stateMachine;
    std::unique_ptr<AmbientSubsystemContext> subsystemContext;
    bool isRunning;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_MODEL_HPP
