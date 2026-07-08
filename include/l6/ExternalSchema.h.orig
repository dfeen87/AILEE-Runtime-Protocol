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
};

struct ExternalClusterView {
    size_t node_count;
    double coherence;
    std::vector<ExternalEnvelope> envelopes;
};

struct ExternalReplayTick {
    uint64_t tick_index;
    ExternalClusterView cluster_view;
    std::string scheduler_state_hash;
    std::string telemetry_json;
};

} // namespace l6
} // namespace ailee
