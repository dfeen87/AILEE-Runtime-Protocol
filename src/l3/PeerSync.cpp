#include "l3/PeerSync.h"
#include <cstring>
#include <algorithm>

#include "protocol/ProtocolFrame.hpp"   // signature verification
#include "network/MainnetDiscovery.hpp" // mainnet peer discovery + verification
#include <openssl/sha.h>

namespace ailee {
namespace l3 {

// ============================================================================
// GLOBAL MAINNET DISCOVERY POINTER
// ============================================================================
// This pointer is bound externally (during system initialization).
// PeerSync uses it to:
//   - register new peers discovered via protocol frames
//   - mark peers as verified after signature validation
//   - support mainnet sync negotiation
// ============================================================================
static MainnetDiscovery* g_discovery = nullptr;

// Bind discovery subsystem to PeerSync
void bind_mainnet_discovery(MainnetDiscovery* d) {
    g_discovery = d;
}

// ============================================================================
// DETERMINISTIC SIGNATURE VERIFICATION
// ============================================================================
// This recreates the exact signed data used by BroadcastEngine::sign_frame()
// and compares it to the provided signature. No randomness, no external state.
// ============================================================================
static bool verify_signature(const ProtocolFrame& pf)
{
    std::string data = pf.frame_id + pf.type + pf.version +
                       pf.node_id + std::to_string(pf.timestamp) +
                       pf.payload;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), hash);

    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    static const char* digits = "0123456789abcdef";

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(digits[(hash[i] >> 4) & 0xF]);
        hex.push_back(digits[hash[i] & 0xF]);
    }

    return (hex == pf.signature);
}

// ============================================================================
// MAIN PEER SYNC COMPUTATION
// ============================================================================
// This function:
//   1. Verifies protocol frame signatures
//   2. Negotiates mainnet sync (peer verification)
//   3. Computes sync status deterministically
//   4. Computes coherence delta
//   5. Returns a PeerSyncState struct
// ============================================================================
PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope
) {
    PeerSyncState state;
    std::memset(&state, 0, sizeof(state));

    // ------------------------------------------------------------------------
    // SIGNATURE VERIFICATION + MAINNET SYNC NEGOTIATION
    // ------------------------------------------------------------------------
    if (remote_envelope.has_protocol_frame) {
        const ProtocolFrame& pf = remote_envelope.protocol_frame;

        bool sig_ok = verify_signature(pf);

        if (!sig_ok) {
            // Invalid signature → deterministic recovery path
            state.sync_status = SyncStatus::NEEDS_RECOVERY;
            state.coherence_delta = -999; // explicit invalid-frame marker
            return state;
        }

        // --------------------------------------------------------------------
        // MAINNET DISCOVERY HOOK
        // --------------------------------------------------------------------
        // If signature is valid:
        //   - ensure peer exists in discovery table
        //   - mark peer as verified
        // This is the core of mainnet sync negotiation.
        // --------------------------------------------------------------------
        if (g_discovery) {
            if (!g_discovery->hasPeer(pf.node_id)) {
                // GossipLayer may later update the address
                g_discovery->addPeer(pf.node_id, "unknown");
            }
            g_discovery->verifyPeer(pf.node_id);
        }
    }

    // ------------------------------------------------------------------------
    // EXISTING SYNC LOGIC (UNCHANGED)
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
// SUMMARY BUILDER (UNCHANGED)
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
