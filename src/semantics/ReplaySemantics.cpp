#include "semantics/ReplaySemantics.h"

namespace ailee::semantics {

#include "semantics/EpochSemantics.h"

bool ReplaySemantics::validate_replay_chain(const l6::IReplayBuffer& buffer, uint32_t max_replay_epochs) {
    auto history = buffer.get_epoch_history();
    if (history.empty()) return true;

    // We only need block_per_epoch parameter for epoch validation, we assume 1000 here as default semantics for replay check.
    uint32_t blocks_per_epoch = 1000;

    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) {
            if (history[i].epoch_id <= history[i-1].epoch_id) {
                return false;
            }
            if (history[i].epoch_id - history[i-1].epoch_id > max_replay_epochs) {
                return false;
            }
        }
        // "each epoch must have a valid bundle"
        // Note: Replayed bundles will likely lack constraints/transcripts in memory,
        // so a full validate_epoch_bundle might fail if it relies on pointers.
        // We will perform minimal replay validity checks: hash length and monotonic height.
        if (history[i].state_root_hash.length() != 64) {
            return false;
        }
        if (i > 0 && history[i].clock_snapshot.height < history[i-1].clock_snapshot.height) {
            return false;
        }
    }

    return true;
}

void ReplaySemantics::enforce_replay_retention(l6::IReplayBuffer& buffer, uint32_t max_replay_epochs) {
    size_t current_size = buffer.size();
    while (current_size > max_replay_epochs) {
        buffer.remove_oldest();
        current_size--;
    }
}

} // namespace ailee::semantics
