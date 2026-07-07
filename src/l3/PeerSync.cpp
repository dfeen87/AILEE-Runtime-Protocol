#include "l3/PeerSync.h"
#include <cstring>
#include <algorithm>

namespace ailee {
namespace l3 {

PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope
) {
    PeerSyncState state;
    std::memset(&state, 0, sizeof(state));

    // Safely copy envelopes to state
    std::memcpy(&state.local_envelope, &local_envelope, sizeof(l2::ExecutionEnvelope));
    std::memcpy(&state.remote_envelope, &remote_envelope, sizeof(GossipEnvelope));

    uint64_t local_epoch = state.local_envelope.context.l1_height;
    uint64_t remote_epoch = state.remote_envelope.remote_summary.epoch_height;

    // Compare roots securely without memory leaks
    bool roots_match = (std::memcmp(
        state.local_envelope.context.state_root_hash,
        state.remote_envelope.remote_summary.state_root_hash,
        32
    ) == 0);

    // Compute coherence delta
    uint64_t local_coherence = state.local_envelope.context.mesh_coherence_score;
    uint64_t remote_coherence = state.remote_envelope.remote_summary.coherence_score;
    state.coherence_delta = static_cast<int64_t>(local_coherence) - static_cast<int64_t>(remote_coherence);

    // Determine sync status deterministically based on defined rules
    if ((remote_epoch + STALE_THRESHOLD < local_epoch) || (local_epoch + STALE_THRESHOLD < remote_epoch)) {
        state.sync_status = SyncStatus::STALE;
    } else if (local_epoch == remote_epoch && roots_match) {
        state.sync_status = SyncStatus::IN_SYNC;
    } else if (local_epoch == remote_epoch && !roots_match) {
        state.sync_status = SyncStatus::NEEDS_RECOVERY;
    } else if (local_epoch > remote_epoch) {
        state.sync_status = SyncStatus::AHEAD;
    } else {
        state.sync_status = SyncStatus::BEHIND;
    }

    return state;
}

PeerSyncSummary summarize_peer_sync(const PeerSyncState& state) {
    PeerSyncSummary summary;
    std::memset(&summary, 0, sizeof(summary));

    summary.local_epoch = state.local_envelope.context.l1_height;
    summary.remote_epoch = state.remote_envelope.remote_summary.epoch_height;
    summary.delta = state.coherence_delta;
    summary.sync_status = static_cast<uint8_t>(state.sync_status);

    return summary;
}

} // namespace l3
} // namespace ailee
