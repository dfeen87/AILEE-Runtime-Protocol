#pragma once

#include <vector>
#include <cstddef>

namespace ailee {
namespace l4 {

struct FederationConfig {
    size_t cluster_count;
    size_t cross_cluster_latency;

    std::vector<std::vector<bool>> routing_matrix;

    static FederationConfig simple(size_t clusters, size_t latency);
};

} // namespace l4
} // namespace ailee
