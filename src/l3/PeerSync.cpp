#include "l3/PeerSync.h"
#include <cstring>
#include <algorithm>

#include "network/MainnetDiscovery.hpp"
#include <openssl/sha.h>

namespace ailee {
namespace l3 {

// ============================================================================
// GLOBAL MAINNET DISCOVERY POINTER
// ============================================================================
static MainnetDiscovery* g_discovery = nullptr;

void bind_mainnet_discovery(MainnetDiscovery* d) {
    g_discovery = d;
}

// ============================================================================
// MAIN PEER SYNC COMPUTATION
// ============================================================================
PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope
) {
    PeerSyncState state;
    std::memset(&state, 0, sizeof(state));

    // ------------------------------------------------------------------------
    // MAINNET DISCOVERY HOOK (BASED ON REMOTE SUMMARY)
    // ------------------------------------------------------------------------
    if (g_discovery) {
        // Use remote node identity fingerprint as a deterministic peer key.
        std::string peerIdHex(
            reinterpret_cast<const char*>(remote_envelope.remote_summary.node_identity_fingerprint),
            32
        );

        if (!g_discovery->hasPeer(peerIdHex)) {
            g_discovery->addPeer(peerIdHex, "unknown");
        }
        g_discovery->verifyPeer(peerIdHex);
    }

    // ------------------------------------------------------------------------
    // EXISTING SYNC LOGIC
    // ------------------------------------------------------------------------
    std::memcpy(&state.local_envelope, &local_envelope, sizeof(l2::ExecutionEnvelope));
    std::memcpy(&state.remote_envelope, &remote_envelope, sizeof(GossipEnvelope));

    uint64_t local_epoch  = state.local_envelope.context.l1_height;
    uint64_t remote_epoch = state.remote_envelope.remote_summary.epoch_height;

    bool roots_match = (std::memcmp(
        state.local_envelope.context.state_root_hash,
        state.remote_envelope.remote_summary.state_root_hash,
        32
    ) == 0);

    uint64_t local_coherence  = state.local_envelope.context.mesh_coherence_score;
    uint64_t remote_coherence = state.remote_envelope.remote_summary.coherence_score;

    state.coherence_delta =
        static_cast<int64_t>(local_coherence) -
        static_cast<int64_t>(remote_coherence);

    // Deterministic sync classification
    if ((remote_epoch + STALE_THRESHOLD < local_epoch) ||
        (local_epoch + STALE_THRESHOLD < remote_epoch)) {
        state.sync_status = SyncStatus::STALE;
    }
    else if (local_epoch == remote_epoch && roots_match) {
        state.sync_status = SyncStatus::IN_SYNC;
    }
    else if (local_epoch == remote_epoch && !roots_match) {
        state.sync_status = SyncStatus::NEEDS_RECOVERY;
    }
    else if (local_epoch > remote_epoch) {
        state.sync_status = SyncStatus::AHEAD;
    }
    else {
        state.sync_status = SyncStatus::BEHIND;
    }

    return state;
}

// ============================================================================
// SUMMARY BUILDER
// ============================================================================
PeerSyncSummary summarize_peer_sync(const PeerSyncState& state) {
    PeerSyncSummary summary;
    std::memset(&summary, 0, sizeof(summary));

    summary.local_epoch  = state.local_envelope.context.l1_height;
    summary.remote_epoch = state.remote_envelope.remote_summary.epoch_height;
    summary.delta        = state.coherence_delta;
    summary.sync_status  = static_cast<uint8_t>(state.sync_status);

    return summary;
}

} // namespace l3
} // namespace ailee
