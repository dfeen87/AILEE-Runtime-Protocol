#include "semantics/ClockSemantics.h"

namespace ailee::semantics {

bool ClockSemantics::is_valid_snapshot(const l6::ClockSnapshot& snapshot, const l6::IReplayBuffer& replay_buffer, EnvironmentType env) {
    if (snapshot.timestamp == 0) return false;

    auto history = replay_buffer.get_epoch_history();
    uint64_t previous_height = 0;
    if (!history.empty()) {
        previous_height = history.back().clock_snapshot.height;
    }

    if (snapshot.height < previous_height) return false;

    if (snapshot.hash.length() != 64) return false;

    if (env == EnvironmentType::PROD && snapshot.source != "Bitcoin") return false;

    return true;
}

bool ClockSemantics::is_epoch_boundary(const l6::ClockSnapshot& snapshot, uint32_t blocks_per_epoch) {
    if (blocks_per_epoch == 0) return false;
    return (snapshot.height % blocks_per_epoch) == 0;
}

} // namespace ailee::semantics
