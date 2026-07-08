#include "l4/ReplayEngine.h"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace ailee {
namespace l4 {

static const char REPLAY_MAGIC[10] = "AILEE-RPL";
static const uint32_t REPLAY_VERSION = 2;

ReplayTick ReplayEngine::step(const l1_sync::ReplayState& previous, const l1_sync::ReplayInput& input) const {
    ReplayTick tick;
    tick.height = previous.current_height;
    tick.clock = input.clock;

    for (const auto& ev : input.events) {
        l1_sync::ReplayEvent re;
        std::memset(&re, 0, sizeof(re));

        switch (ev.type) {
            case l1_sync::SyncEventType::HeaderApplied:
                re.type = l1_sync::ReplayEventType::HeaderApplied;
                re.height = ev.height;
                re.block_hash = ev.block_hash;
                tick.height = ev.height;
                break;
            case l1_sync::SyncEventType::ReorgDetected:
                re.type = l1_sync::ReplayEventType::ReorgRollback;
                re.height = ev.reorg_target_height; // Rollback target
                re.block_hash = ev.block_hash;
                tick.height = ev.reorg_target_height;
                break;
            case l1_sync::SyncEventType::MempoolDeltaApplied:
                re.type = l1_sync::ReplayEventType::MempoolUpdate;
                re.height = tick.height;
                re.block_hash = ev.block_hash;
                re.txid = ev.txid;
                break;
        }

        tick.replay_events.push_back(re);
    }

    tick.view.clock = tick.clock;
    tick.view.replay_events = tick.replay_events;

    return tick;
}

void ReplayEngine::write_replay_file(const std::string& path, const ReplayBuffer& replay_buffer) const {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Could not open replay file for writing: " + path);
    }

    ofs.write(REPLAY_MAGIC, 10);
    ofs.write(reinterpret_cast<const char*>(&REPLAY_VERSION), sizeof(REPLAY_VERSION));
    
    uint64_t tick_count = replay_buffer.scheduler_snapshots.size();
    ofs.write(reinterpret_cast<const char*>(&tick_count), sizeof(tick_count));

    for (uint64_t i = 0; i < tick_count; ++i) {
        ofs.write(reinterpret_cast<const char*>(&replay_buffer.scheduler_snapshots[i]), sizeof(ReplaySchedulerSnapshot));
        
        const auto& v_snap = replay_buffer.view_snapshots[i];
        
        uint64_t nodes_size = v_snap.nodes.size();
        ofs.write(reinterpret_cast<const char*>(&nodes_size), sizeof(nodes_size));
        for (const auto& node : v_snap.nodes) {
            // Write fixed part safely (without hitting the internal std::vector pointers which vary)
            // Just copying the raw struct doesn't work because it contains a vector.
            // We have to selectively copy the POD elements of ClusterNodeState or just zero out the vector before writing
            ClusterNodeState pod_node = node;
            
            // clear the vector inside our local pod_node so we just write an empty vector footprint deterministically
            new (&pod_node.peer_sync_states) std::vector<l3::PeerSyncState>(); 
            ofs.write(reinterpret_cast<const char*>(&pod_node), sizeof(ClusterNodeState));
            
            // Write vector part
            uint64_t sync_size = node.peer_sync_states.size();
            ofs.write(reinterpret_cast<const char*>(&sync_size), sizeof(sync_size));
            if (sync_size > 0) {
                ofs.write(reinterpret_cast<const char*>(node.peer_sync_states.data()), sync_size * sizeof(l3::PeerSyncState));
            }
        }
        
        uint64_t env_size = v_snap.mesh_envelopes.size();
        ofs.write(reinterpret_cast<const char*>(&env_size), sizeof(env_size));
        if (env_size > 0) {
            ofs.write(reinterpret_cast<const char*>(v_snap.mesh_envelopes.data()), env_size * sizeof(MeshPropagationEnvelope));
        }

        uint64_t msg_size = v_snap.transport_queue.messages.size();
        ofs.write(reinterpret_cast<const char*>(&msg_size), sizeof(msg_size));
        if (msg_size > 0) {
            ofs.write(reinterpret_cast<const char*>(v_snap.transport_queue.messages.data()), msg_size * sizeof(TransportMessage));
        }
        
        ofs.write(reinterpret_cast<const char*>(&v_snap.transport_queue.padding), sizeof(v_snap.transport_queue.padding));
        
        ofs.write(reinterpret_cast<const char*>(&v_snap.total_nodes), sizeof(v_snap.total_nodes));
        ofs.write(reinterpret_cast<const char*>(&v_snap.total_steps), sizeof(v_snap.total_steps));
        ofs.write(reinterpret_cast<const char*>(&v_snap.coherence_summary), sizeof(ClusterCoherenceSummary));

        ofs.write(reinterpret_cast<const char*>(&v_snap.clock), sizeof(l1_sync::BitcoinClockState));

        uint64_t events_size = v_snap.replay_events.size();
        ofs.write(reinterpret_cast<const char*>(&events_size), sizeof(events_size));
        for (const auto& ev : v_snap.replay_events) {
            uint8_t type_val = static_cast<uint8_t>(ev.type);
            ofs.write(reinterpret_cast<const char*>(&type_val), sizeof(type_val));
            ofs.write(reinterpret_cast<const char*>(&ev.height), sizeof(ev.height));
            ofs.write(reinterpret_cast<const char*>(ev.block_hash.data()), ev.block_hash.size());
            ofs.write(reinterpret_cast<const char*>(ev.txid.data()), ev.txid.size());
        }

        ofs.write(reinterpret_cast<const char*>(&replay_buffer.telemetry_snapshots[i]), sizeof(TelemetrySample));
    }
}

void ReplayEngine::load_replay_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not open replay file for reading: " + path);
    }

    char magic[10];
    ifs.read(magic, 10);
    if (std::memcmp(magic, REPLAY_MAGIC, 10) != 0) {
        throw std::runtime_error("Invalid replay magic bytes");
    }

    uint32_t version;
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version > REPLAY_VERSION) {
        throw std::runtime_error("Unsupported replay version");
    }

    uint64_t tick_count;
    ifs.read(reinterpret_cast<char*>(&tick_count), sizeof(tick_count));

    buffer.scheduler_snapshots.resize(tick_count);
    buffer.view_snapshots.resize(tick_count);
    buffer.telemetry_snapshots.resize(tick_count);
    
    for (uint64_t i = 0; i < tick_count; ++i) {
        ifs.read(reinterpret_cast<char*>(&buffer.scheduler_snapshots[i]), sizeof(ReplaySchedulerSnapshot));
        
        auto& v_snap = buffer.view_snapshots[i];
        
        uint64_t nodes_size;
        ifs.read(reinterpret_cast<char*>(&nodes_size), sizeof(nodes_size));
        v_snap.nodes.resize(nodes_size);
        for (uint64_t j = 0; j < nodes_size; ++j) {
            ClusterNodeState raw_node;
            // Placement new to properly initialize the vector inside
            new (&raw_node) ClusterNodeState();
            
            // Read over the struct
            ifs.read(reinterpret_cast<char*>(&raw_node), sizeof(ClusterNodeState));
            
            // The vector inside raw_node is now corrupted with whatever was on disk.
            // We need to re-initialize it cleanly.
            new (&raw_node.peer_sync_states) std::vector<l3::PeerSyncState>();
            
            uint64_t sync_size;
            ifs.read(reinterpret_cast<char*>(&sync_size), sizeof(sync_size));
            raw_node.peer_sync_states.resize(sync_size);
            if (sync_size > 0) {
                ifs.read(reinterpret_cast<char*>(raw_node.peer_sync_states.data()), sync_size * sizeof(l3::PeerSyncState));
            }
            
            v_snap.nodes[j] = std::move(raw_node);
        }
        
        uint64_t env_size;
        ifs.read(reinterpret_cast<char*>(&env_size), sizeof(env_size));
        v_snap.mesh_envelopes.resize(env_size);
        if (env_size > 0) {
            ifs.read(reinterpret_cast<char*>(v_snap.mesh_envelopes.data()), env_size * sizeof(MeshPropagationEnvelope));
        }

        uint64_t msg_size;
        ifs.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
        v_snap.transport_queue.messages.resize(msg_size);
        if (msg_size > 0) {
            ifs.read(reinterpret_cast<char*>(v_snap.transport_queue.messages.data()), msg_size * sizeof(TransportMessage));
        }
        
        ifs.read(reinterpret_cast<char*>(&v_snap.transport_queue.padding), sizeof(v_snap.transport_queue.padding));

        ifs.read(reinterpret_cast<char*>(&v_snap.total_nodes), sizeof(v_snap.total_nodes));
        ifs.read(reinterpret_cast<char*>(&v_snap.total_steps), sizeof(v_snap.total_steps));
        ifs.read(reinterpret_cast<char*>(&v_snap.coherence_summary), sizeof(ClusterCoherenceSummary));

        if (version >= 2) {
            ifs.read(reinterpret_cast<char*>(&v_snap.clock), sizeof(l1_sync::BitcoinClockState));

            uint64_t events_size;
            ifs.read(reinterpret_cast<char*>(&events_size), sizeof(events_size));
            v_snap.replay_events.resize(events_size);
            for (uint64_t e = 0; e < events_size; ++e) {
                uint8_t type_val;
                ifs.read(reinterpret_cast<char*>(&type_val), sizeof(type_val));
                v_snap.replay_events[e].type = static_cast<l1_sync::ReplayEventType>(type_val);
                ifs.read(reinterpret_cast<char*>(&v_snap.replay_events[e].height), sizeof(v_snap.replay_events[e].height));
                ifs.read(reinterpret_cast<char*>(v_snap.replay_events[e].block_hash.data()), v_snap.replay_events[e].block_hash.size());
                ifs.read(reinterpret_cast<char*>(v_snap.replay_events[e].txid.data()), v_snap.replay_events[e].txid.size());
            }
        } else {
            std::memset(&v_snap.clock, 0, sizeof(v_snap.clock));
            v_snap.replay_events.clear();
        }

        ifs.read(reinterpret_cast<char*>(&buffer.telemetry_snapshots[i]), sizeof(TelemetrySample));
    }
}

bool ReplayEngine::replay_tick(
    size_t tick_index,
    DeterministicSchedulerState& out_scheduler,
    ClusterView& out_view,
    TelemetrySample& out_telemetry
) const {
    if (tick_index >= buffer.scheduler_snapshots.size()) return false;

    out_scheduler = buffer.scheduler_snapshots[tick_index].state;
    
    const auto& v_snap = buffer.view_snapshots[tick_index];
    out_view.nodes = v_snap.nodes;
    out_view.mesh_envelopes = v_snap.mesh_envelopes;
    out_view.transport_queue = v_snap.transport_queue;
    out_view.total_nodes = v_snap.total_nodes;
    out_view.total_steps = v_snap.total_steps;
    out_view.coherence_summary = v_snap.coherence_summary;
    out_view.clock = v_snap.clock;
    out_view.replay_events = v_snap.replay_events;
    
    out_telemetry = buffer.telemetry_snapshots[tick_index];

    return true;
}

bool ReplayEngine::verify_tick(
    size_t tick_index,
    const DeterministicSchedulerState& scheduler,
    const ClusterView& view,
    const TelemetrySample& telemetry
) const {
    if (tick_index >= buffer.scheduler_snapshots.size()) return false;
    
    if (std::memcmp(&scheduler, &buffer.scheduler_snapshots[tick_index].state, sizeof(scheduler)) != 0) return false;
    if (std::memcmp(&telemetry, &buffer.telemetry_snapshots[tick_index], sizeof(telemetry)) != 0) return false;
    
    const auto& v_snap = buffer.view_snapshots[tick_index];
    
    if (view.total_nodes != v_snap.total_nodes) return false;
    if (view.total_steps != v_snap.total_steps) return false;
    if (std::memcmp(&view.coherence_summary, &v_snap.coherence_summary, sizeof(ClusterCoherenceSummary)) != 0) return false;
    
    if (std::memcmp(&view.clock, &v_snap.clock, sizeof(l1_sync::BitcoinClockState)) != 0) return false;
    if (view.replay_events.size() != v_snap.replay_events.size()) return false;
    for (size_t i = 0; i < view.replay_events.size(); ++i) {
        if (view.replay_events[i].type != v_snap.replay_events[i].type) return false;
        if (view.replay_events[i].height != v_snap.replay_events[i].height) return false;
        if (std::memcmp(view.replay_events[i].block_hash.data(), v_snap.replay_events[i].block_hash.data(), 32) != 0) return false;
        if (std::memcmp(view.replay_events[i].txid.data(), v_snap.replay_events[i].txid.data(), 32) != 0) return false;
    }

    if (view.nodes.size() != v_snap.nodes.size()) return false;
    for (size_t i = 0; i < view.nodes.size(); ++i) {
        ClusterNodeState node_pod = view.nodes[i];
        new (&node_pod.peer_sync_states) std::vector<l3::PeerSyncState>(); 
        
        ClusterNodeState v_snap_node_pod = v_snap.nodes[i];
        new (&v_snap_node_pod.peer_sync_states) std::vector<l3::PeerSyncState>();
        
        if (std::memcmp(&node_pod, &v_snap_node_pod, sizeof(ClusterNodeState)) != 0) return false;
        if (view.nodes[i].peer_sync_states.size() != v_snap.nodes[i].peer_sync_states.size()) return false;
        if (view.nodes[i].peer_sync_states.size() > 0) {
            if (std::memcmp(view.nodes[i].peer_sync_states.data(), v_snap.nodes[i].peer_sync_states.data(), view.nodes[i].peer_sync_states.size() * sizeof(l3::PeerSyncState)) != 0) return false;
        }
    }
    
    if (view.mesh_envelopes.size() != v_snap.mesh_envelopes.size()) return false;
    if (view.mesh_envelopes.size() > 0) {
        if (std::memcmp(view.mesh_envelopes.data(), v_snap.mesh_envelopes.data(), view.mesh_envelopes.size() * sizeof(MeshPropagationEnvelope)) != 0) return false;
    }
    
    if (view.transport_queue.messages.size() != v_snap.transport_queue.messages.size()) return false;
    if (view.transport_queue.messages.size() > 0) {
        if (std::memcmp(view.transport_queue.messages.data(), v_snap.transport_queue.messages.data(), view.transport_queue.messages.size() * sizeof(TransportMessage)) != 0) return false;
    }
    
    return true;
}

} // namespace l4
} // namespace ailee
