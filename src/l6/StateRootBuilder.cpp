#include "l6/StateRootBuilder.h"
#include <openssl/sha.h>
#include <cstring>
#include <vector>

namespace ailee {
namespace l6 {

namespace {

void write_bytes(std::vector<uint8_t>& out, const void* data, size_t size) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    out.insert(out.end(), ptr, ptr + size);
}

void write_u8(std::vector<uint8_t>& out, uint8_t val) {
    out.push_back(val);
}

void write_u32_le(std::vector<uint8_t>& out, uint32_t val) {
    uint8_t buf[4];
    buf[0] = static_cast<uint8_t>(val & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    write_bytes(out, buf, 4);
}

void write_u64_be(std::vector<uint8_t>& out, uint64_t val) {
    uint8_t buf[8];
    buf[0] = static_cast<uint8_t>((val >> 56) & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 48) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 40) & 0xFF);
    buf[3] = static_cast<uint8_t>((val >> 32) & 0xFF);
    buf[4] = static_cast<uint8_t>((val >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[6] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[7] = static_cast<uint8_t>(val & 0xFF);
    write_bytes(out, buf, 8);
}

void write_u64_le(std::vector<uint8_t>& out, uint64_t val) {
    uint8_t buf[8];
    buf[0] = static_cast<uint8_t>(val & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    buf[4] = static_cast<uint8_t>((val >> 32) & 0xFF);
    buf[5] = static_cast<uint8_t>((val >> 40) & 0xFF);
    buf[6] = static_cast<uint8_t>((val >> 48) & 0xFF);
    buf[7] = static_cast<uint8_t>((val >> 56) & 0xFF);
    write_bytes(out, buf, 8);
}

void write_i64_le(std::vector<uint8_t>& out, int64_t val) {
    // Standard two's complement behavior
    write_u64_le(out, static_cast<uint64_t>(val));
}

// Convert float to fixed-precision integer for deterministic serialization
uint64_t double_to_fixed(double val) {
    // Multiply by 1000000 to keep 6 decimal places of precision deterministically
    return static_cast<uint64_t>(val * 1000000.0);
}

void serialize_epoch_state(std::vector<uint8_t>& out, const l2::EpochState& state) {
    write_bytes(out, state.epoch_hash, 32);
    write_u64_le(out, state.epoch_number);
    write_u64_le(out, state.l1_height);
    write_bytes(out, state.l1_anchor_hash, 32);
    write_u32_le(out, state.protocol_version);
    write_bytes(out, state.node_identity_hash, 32);
}

void serialize_state_root(std::vector<uint8_t>& out, const l2::StateRoot& root) {
    write_bytes(out, root.root_hash, 32);
    write_u64_le(out, root.l1_height_basis);
    write_u32_le(out, root.ingestion_count);
    write_u32_le(out, root.coherence_score);
}

void serialize_recovery_payload(std::vector<uint8_t>& out, const l2::RecoveryPayload& payload) {
    write_bytes(out, payload.payload_hash, 32);
    write_bytes(out, payload.target_state_root, 32);
    write_bytes(out, payload.target_epoch_hash, 32);
    write_u64_le(out, payload.l1_height);
    write_bytes(out, payload.node_identity_hash, 32);
}

void serialize_execution_context(std::vector<uint8_t>& out, const l2::ExecutionContext& ctx) {
    write_bytes(out, ctx.context_hash, 32);
    write_bytes(out, ctx.epoch_hash, 32);
    write_bytes(out, ctx.state_root_hash, 32);
    write_bytes(out, ctx.node_identity_hash, 32);
    write_u64_le(out, ctx.l1_height);
    write_u32_le(out, ctx.mesh_coherence_score);
}

void serialize_execution_envelope(std::vector<uint8_t>& out, const l2::ExecutionEnvelope& env) {
    serialize_execution_context(out, env.context);
    write_bytes(out, env.signature_or_commitment, 64);
}

void serialize_engine_state(std::vector<uint8_t>& out, const l2::EngineState& state) {
    serialize_epoch_state(out, state.epoch);
    serialize_state_root(out, state.state_root);
    serialize_recovery_payload(out, state.recovery);
    serialize_execution_context(out, state.context);
    write_u64_le(out, state.step_counter);
}

void serialize_gossip_summary(std::vector<uint8_t>& out, const l3::GossipSummary& summary) {
    write_bytes(out, summary.node_identity_fingerprint, 32);
    write_bytes(out, summary.state_root_hash, 32);
    write_bytes(out, summary.epoch_hash, 32);
    write_u64_le(out, summary.epoch_height);
    write_u64_le(out, summary.coherence_score);
    write_u8(out, summary.flags);
}

void serialize_gossip_envelope(std::vector<uint8_t>& out, const l3::GossipEnvelope& env) {
    serialize_gossip_summary(out, env.remote_summary);
    write_u64_le(out, env.received_sequence);
    write_bytes(out, env.normalized_hash, 32);
}

void serialize_peer_sync_state(std::vector<uint8_t>& out, const l3::PeerSyncState& state) {
    serialize_execution_envelope(out, state.local_envelope);
    serialize_gossip_envelope(out, state.remote_envelope);
    write_i64_le(out, state.coherence_delta);
    write_u8(out, static_cast<uint8_t>(state.sync_status));
}

void serialize_mesh_epoch(std::vector<uint8_t>& out, const l4::MeshEpoch& epoch) {
    write_u64_le(out, epoch.epoch_height);
    write_bytes(out, epoch.epoch_hash, 32);
    write_bytes(out, epoch.mesh_state_root, 32);
}

void serialize_mesh_anchor(std::vector<uint8_t>& out, const l4::MeshAnchor& anchor) {
    write_u64_le(out, anchor.anchor_id);
    serialize_mesh_epoch(out, anchor.epoch);
    write_u64_le(out, anchor.total_nodes);
    write_u64_le(out, anchor.in_sync_nodes);
    write_u64_le(out, anchor.recovered_nodes);
    write_bytes(out, anchor.anchor_hash, 32);
}

void serialize_mesh_prop_envelope(std::vector<uint8_t>& out, const l4::MeshPropagationEnvelope& env) {
    write_u64_le(out, env.source_node_id_hash);
    serialize_mesh_anchor(out, env.anchor);
    write_bytes(out, env.propagation_hash, 32);
}

void serialize_transport_message(std::vector<uint8_t>& out, const l4::TransportMessage& msg) {
    write_u64_le(out, msg.source_node_id_hash);
    write_u64_le(out, msg.dest_node_id_hash);
    write_u64_le(out, msg.epoch_height);
    write_u8(out, msg.message_type);
    write_bytes(out, msg.payload, 128);
    write_bytes(out, msg.message_hash, 32);
}

void serialize_coherence_summary(std::vector<uint8_t>& out, const l4::ClusterCoherenceSummary& summary) {
    write_u64_le(out, summary.in_sync_count);
    write_u64_le(out, summary.ahead_count);
    write_u64_le(out, summary.behind_count);
    write_u64_le(out, summary.needs_recovery_count);
    write_u64_le(out, summary.stale_count);
    write_u64_le(out, summary.global_coherence_score);
    write_u64_le(out, summary.recovered_nodes_count);
    write_u64_le(out, summary.unrecoverable_nodes_count);
    write_u64_le(out, summary.consistent_state_root_nodes);
    write_u64_le(out, summary.inconsistent_state_root_nodes);
}

} // anonymous namespace

std::array<uint8_t, 32> StateRootBuilder::build_state_root(
    const l4::ReplayBuffer& replay,
    const l4::ClusterFederationView& federation
) {
    // 1. Replay hash
    std::array<uint8_t, 32> replay_hash = {0};
    if (!replay.compressed_ticks.empty()) {
        const auto& last_tick = replay.compressed_ticks.back();
        SHA256(last_tick.data(), last_tick.size(), replay_hash.data());
    }

    // 2. Federation hash
    std::vector<uint8_t> fed_buf;

    // Serialize cluster_views deterministically
    write_u64_le(fed_buf, federation.cluster_views.size());
    for (const auto& cv : federation.cluster_views) {
        write_u64_le(fed_buf, cv.nodes.size());
        for (const auto& node : cv.nodes) {
            serialize_engine_state(fed_buf, node.engine_state);
            serialize_execution_envelope(fed_buf, node.last_envelope);
            serialize_gossip_summary(fed_buf, node.last_gossip_summary);
            write_u64_le(fed_buf, node.node_id_hash);
            write_u64_le(fed_buf, node.step_counter);
            write_u8(fed_buf, static_cast<uint8_t>(node.state_root_status));

            write_u64_le(fed_buf, node.peer_sync_states.size());
            for (const auto& sync : node.peer_sync_states) {
                serialize_peer_sync_state(fed_buf, sync);
            }
        }

        write_u64_le(fed_buf, cv.mesh_envelopes.size());
        for (const auto& env : cv.mesh_envelopes) {
            serialize_mesh_prop_envelope(fed_buf, env);
        }

        write_u64_le(fed_buf, cv.total_nodes);
        write_u64_le(fed_buf, cv.total_steps);

        write_u64_le(fed_buf, cv.transport_queue.messages.size());
        for (const auto& msg : cv.transport_queue.messages) {
            serialize_transport_message(fed_buf, msg);
        }

        serialize_coherence_summary(fed_buf, cv.coherence_summary);
    }

    // Serialize envelope_stats
    write_u64_le(fed_buf, federation.envelope_stats.in_flight);
    write_u64_le(fed_buf, federation.envelope_stats.delivered);
    write_u64_le(fed_buf, federation.envelope_stats.pending);

    // Serialize coherence_summary (deterministic fixed precision)
    write_u64_le(fed_buf, double_to_fixed(federation.coherence_summary.average_coherence));
    write_u64_le(fed_buf, double_to_fixed(federation.coherence_summary.min_coherence));
    write_u64_le(fed_buf, double_to_fixed(federation.coherence_summary.max_coherence));

    std::array<uint8_t, 32> federation_hash = {0};
    SHA256(fed_buf.data(), fed_buf.size(), federation_hash.data());

    // 3. Global coherence score
    uint64_t coherence_score = static_cast<uint64_t>(federation.coherence_summary.average_coherence);

    // 4. Final state root buffer
    std::vector<uint8_t> final_buf;
    final_buf.reserve(1 + 32 + 32 + 8);

    uint8_t version = 0x01;
    write_u8(final_buf, version);
    write_bytes(final_buf, replay_hash.data(), 32);
    write_bytes(final_buf, federation_hash.data(), 32);
    write_u64_be(final_buf, coherence_score);

    std::array<uint8_t, 32> state_root = {0};
    SHA256(final_buf.data(), final_buf.size(), state_root.data());

    return state_root;
}

} // namespace l6
} // namespace ailee
