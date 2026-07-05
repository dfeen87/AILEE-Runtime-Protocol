#ifndef AMBIENT_AI_RUNTIME_STATE_MACHINE_HPP
#define AMBIENT_AI_RUNTIME_STATE_MACHINE_HPP

#include "ambient_ai_runtime_phases.hpp"
#include <vector>
#include <cstdint>

namespace ailee {
namespace runtime {

struct AmbientRuntimeState {
    AmbientRuntimePhase currentPhase;
    uint64_t currentLogicalTimestamp;
    uint64_t lastProcessedBitcoinHeight;

    bool isHealthy;
    uint32_t consecutiveFailures;
};

class AmbientRuntimeStateMachine {
public:
    AmbientRuntimeStateMachine();

    AmbientRuntimeState getCurrentState() const;

    // Deterministically attempts to step the phase based on current context
    bool applyTransition(const AmbientRuntimePhaseTransition& transition);

    // Handles subsystem faults gracefully without crashing AILEE CORE
    void triggerRecovery(uint64_t logicalTimestamp);

private:
    AmbientRuntimeState state;

    bool checkPhaseInvariants(AmbientRuntimePhase phase) const;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_STATE_MACHINE_HPP
