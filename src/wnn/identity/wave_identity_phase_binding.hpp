#pragma once

#include <vector>
#include <cstdint>
#include <atomic>
#include <map>

namespace ailee {
namespace mesh_legacy {
    using AileeTrustScore = double;
} // namespace mesh_legacy

namespace wnn {
namespace space {
    using AnchorId = uint32_t;
} // namespace space

using PeerId = std::vector<uint8_t>;

struct CoherenceCluster {
    std::vector<space::AnchorId> anchors;
};

struct KnownPeer {
    std::vector<uint8_t> signature;
    double spectral_frequency;
    mesh_legacy::AileeTrustScore trust_score;
    std::atomic<double> latency_ema;
    std::atomic<double> jitter_ema;
    long double phase_theta;
};

class CoherenceEngine {
public:
    void evaluate_clusters(const std::vector<CoherenceCluster>& clusters, std::map<space::AnchorId, KnownPeer>& anchors);
};

} // namespace wnn
} // namespace ailee
