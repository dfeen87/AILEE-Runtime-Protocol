#include "l4/FederationConfig.h"

namespace ailee {
namespace l4 {

FederationConfig FederationConfig::simple(size_t clusters, size_t latency) {
    FederationConfig cfg;
    cfg.cluster_count = clusters;
    cfg.cross_cluster_latency = latency;
    cfg.routing_matrix.resize(clusters, std::vector<bool>(clusters, true));
    return cfg;
}

} // namespace l4
} // namespace ailee
