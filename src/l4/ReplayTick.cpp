#include "l4/ReplayTick.h"
#include <cstring>
#include <stdexcept>

namespace ailee {
namespace l4 {

namespace {

void write_bytes(std::vector<uint8_t>& out, const void* data, size_t size) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    out.insert(out.end(), ptr, ptr + size);
}

void write_u64(std::vector<uint8_t>& out, uint64_t val) {
    write_bytes(out, &val, sizeof(val)); // Assuming little-endian platform for now, consistent with ReplayEngine
}

template<typename T>
void read_bytes(const std::vector<uint8_t>& in, size_t& offset, T* out, size_t size) {
    if (offset + size > in.size()) {
        throw std::runtime_error("Buffer underflow during deserialize");
    }
    std::memcpy(out, in.data() + offset, size);
    offset += size;
}

uint64_t read_u64(const std::vector<uint8_t>& in, size_t& offset) {
    uint64_t val;
    read_bytes(in, offset, &val, sizeof(val));
    return val;
}

} // anonymous namespace

std::vector<uint8_t> ReplayTick::serialize() const {
    std::vector<uint8_t> out;

    uint32_t version = 2;
    write_bytes(out, &version, sizeof(version));

    // Serialize Scheduler State
    write_bytes(out, &scheduler_state, sizeof(DeterministicSchedulerState));

    // Serialize ClusterView
    write_u64(out, view.nodes.size());
    for (const auto& node : view.nodes) {
        // Explicitly serialize scalar properties to avoid copying std::vector internals
        write_bytes(out, &node.engine_state, sizeof(node.engine_state));
        write_bytes(out, &node.last_envelope, sizeof(node.last_envelope));
        write_bytes(out, &node.last_gossip_summary, sizeof(node.last_gossip_summary));
        write_u64(out, node.node_id_hash);
        write_u64(out, node.step_counter);
        write_bytes(out, &node.state_root_status, sizeof(node.state_root_status));
        write_bytes(out, node.padding, sizeof(node.padding));

        // Write dynamic part
        write_u64(out, node.peer_sync_states.size());
        if (!node.peer_sync_states.empty()) {
            write_bytes(out, node.peer_sync_states.data(), node.peer_sync_states.size() * sizeof(l3::PeerSyncState));
        }
    }

    write_u64(out, view.mesh_envelopes.size());
    if (!view.mesh_envelopes.empty()) {
        write_bytes(out, view.mesh_envelopes.data(), view.mesh_envelopes.size() * sizeof(MeshPropagationEnvelope));
    }

    write_u64(out, view.transport_queue.messages.size());
    if (!view.transport_queue.messages.empty()) {
        write_bytes(out, view.transport_queue.messages.data(), view.transport_queue.messages.size() * sizeof(TransportMessage));
    }
    write_bytes(out, &view.transport_queue.padding, sizeof(view.transport_queue.padding));

    write_u64(out, view.total_nodes);
    write_u64(out, view.total_steps);
    write_bytes(out, &view.coherence_summary, sizeof(ClusterCoherenceSummary));

    // Serialize Telemetry Sample
    write_bytes(out, &telemetry, sizeof(TelemetrySample));

    // Serialize Version 2 Fields
    write_u64(out, height);
    write_bytes(out, &clock, sizeof(l1_sync::BitcoinClockState));

    write_u64(out, replay_events.size());
    for (const auto& ev : replay_events) {
        uint8_t type_val = static_cast<uint8_t>(ev.type);
        write_bytes(out, &type_val, sizeof(type_val));
        write_u64(out, ev.height);
        write_bytes(out, ev.block_hash.data(), ev.block_hash.size());
        write_bytes(out, ev.txid.data(), ev.txid.size());
    }

    return out;
}

ReplayTick ReplayTick::deserialize(const std::vector<uint8_t>& raw) {
    ReplayTick tick;
    size_t offset = 0;

    uint32_t version;
    read_bytes(raw, offset, &version, sizeof(version));

    // Deserialize Scheduler State
    read_bytes(raw, offset, &tick.scheduler_state, sizeof(DeterministicSchedulerState));

    // Deserialize ClusterView
    uint64_t nodes_size = read_u64(raw, offset);
    tick.view.nodes.resize(nodes_size);
    for (uint64_t i = 0; i < nodes_size; ++i) {
        ClusterNodeState raw_node; // Default constructs the vector

        // Explicitly deserialize scalar properties
        read_bytes(raw, offset, &raw_node.engine_state, sizeof(raw_node.engine_state));
        read_bytes(raw, offset, &raw_node.last_envelope, sizeof(raw_node.last_envelope));
        read_bytes(raw, offset, &raw_node.last_gossip_summary, sizeof(raw_node.last_gossip_summary));
        raw_node.node_id_hash = read_u64(raw, offset);
        raw_node.step_counter = read_u64(raw, offset);
        read_bytes(raw, offset, &raw_node.state_root_status, sizeof(raw_node.state_root_status));
        read_bytes(raw, offset, raw_node.padding, sizeof(raw_node.padding));

        uint64_t sync_size = read_u64(raw, offset);
        raw_node.peer_sync_states.resize(sync_size);
        if (sync_size > 0) {
            read_bytes(raw, offset, raw_node.peer_sync_states.data(), sync_size * sizeof(l3::PeerSyncState));
        }

        tick.view.nodes[i] = std::move(raw_node);
    }

    uint64_t env_size = read_u64(raw, offset);
    tick.view.mesh_envelopes.resize(env_size);
    if (env_size > 0) {
        read_bytes(raw, offset, tick.view.mesh_envelopes.data(), env_size * sizeof(MeshPropagationEnvelope));
    }

    uint64_t msg_size = read_u64(raw, offset);
    tick.view.transport_queue.messages.resize(msg_size);
    if (msg_size > 0) {
        read_bytes(raw, offset, tick.view.transport_queue.messages.data(), msg_size * sizeof(TransportMessage));
    }
    read_bytes(raw, offset, &tick.view.transport_queue.padding, sizeof(tick.view.transport_queue.padding));

    tick.view.total_nodes = read_u64(raw, offset);
    tick.view.total_steps = read_u64(raw, offset);
    read_bytes(raw, offset, &tick.view.coherence_summary, sizeof(ClusterCoherenceSummary));

    // Deserialize Telemetry Sample
    read_bytes(raw, offset, &tick.telemetry, sizeof(TelemetrySample));

    if (version >= 2) {
        tick.height = read_u64(raw, offset);
        read_bytes(raw, offset, &tick.clock, sizeof(l1_sync::BitcoinClockState));

        uint64_t events_size = read_u64(raw, offset);
        tick.replay_events.resize(events_size);
        for (uint64_t i = 0; i < events_size; ++i) {
            uint8_t type_val;
            read_bytes(raw, offset, &type_val, sizeof(type_val));
            tick.replay_events[i].type = static_cast<l1_sync::ReplayEventType>(type_val);
            tick.replay_events[i].height = read_u64(raw, offset);
            read_bytes(raw, offset, tick.replay_events[i].block_hash.data(), tick.replay_events[i].block_hash.size());
            read_bytes(raw, offset, tick.replay_events[i].txid.data(), tick.replay_events[i].txid.size());
        }
    } else {
        tick.height = 0;
        std::memset(&tick.clock, 0, sizeof(tick.clock));
        tick.replay_events.clear();
    }

    return tick;
}

} // namespace l4
} // namespace ailee
