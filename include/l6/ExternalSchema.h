#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ailee {
namespace l6 {

struct ExternalEnvelope {
    uint64_t id;
    uint64_t timestamp;
    std::string payload_hash;
    std::string type = "";   // V35 extension
    std::string status = ""; // V35 extension
};

struct ExternalBitcoinClockState {
    uint64_t height;
    double consensus_time;
    double interval_seconds;
};

struct ExternalReplayEvent {
    uint8_t type;
    uint64_t height;
    std::string block_hash;
    std::string txid;
};

struct ExternalClusterView {
    size_t node_count;
    double coherence;
    std::vector<ExternalEnvelope> envelopes;
    ExternalBitcoinClockState clock;
    std::vector<ExternalReplayEvent> replay_events;
};

struct ExternalReplayTick {
    uint64_t tick_index;
    ExternalClusterView cluster_view;
    std::string scheduler_state_hash;
    std::string telemetry_json;
    std::string replay_phase = "";      // V35 extension
    std::string replay_mode_state = ""; // V35 extension
};

// V35 State-Projection Structs
struct ExternalActiveSession {
    std::string session_id;
    std::string status;
};

struct ExternalBroadcastQueue {
    uint64_t pending_count;
    std::string queue_id;
};

struct ExternalStateSnapshot {
    std::vector<ExternalActiveSession> active_sessions;
    std::vector<ExternalBroadcastQueue> broadcast_queues;
    uint64_t current_tick_index;
    uint64_t global_tick_count;
    bool replay_active;
    uint64_t subsystem_tick_count;
    uint64_t total_ticks;
};

} // namespace l6
} // namespace ailee
