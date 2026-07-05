#ifndef AMBIENT_AI_RUNTIME_PHASES_HPP
#define AMBIENT_AI_RUNTIME_PHASES_HPP

#include <cstdint>

namespace ailee {
namespace runtime {

enum class AmbientRuntimePhase : uint8_t {
    INIT = 0,
    LOAD_IDENTITY = 1,
    LOAD_ENERGY = 2,
    LOAD_NODE_MODE = 3,
    LOAD_MESH = 4,
    LOAD_L2_CONSENSUS = 5,
    EXECUTE_EPOCH = 6,
    MERKLEIZE = 7,
    ANCHOR = 8,
    FINALIZE = 9,
    RECOVERY = 10
};

struct AmbientRuntimePhaseTransition {
    AmbientRuntimePhase fromPhase;
    AmbientRuntimePhase toPhase;
    uint64_t logicalTimestamp;
    uint64_t targetBitcoinHeight; // Required for execution boundary changes

    // Validates deterministic transitions without wall-clock reliance
    bool isValid() const;
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_PHASES_HPP
