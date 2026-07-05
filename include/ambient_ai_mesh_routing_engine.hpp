#ifndef AMBIENT_AI_MESH_ROUTING_ENGINE_HPP
#define AMBIENT_AI_MESH_ROUTING_ENGINE_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <set>

// Forward declarations to allow compiling out-of-order architecture files
namespace ailee {
namespace ambient_mesh {
struct AmbientMeshMessage;
struct NeighborEntry;
}
}

namespace ailee {
namespace ambient_mesh {

class AmbientMeshRoutingEngine {
public:
    // Selects the next hop peers deterministically.
    // Fan-out is strictly bounded by AmbientMeshLimits::MAX_FAN_OUT_DEGREE.
    // Priority is based on NeighborEntry's operator< (energy score, then peerId).
    std::vector<std::string> selectNextHops(
        const AmbientMeshMessage& message,
        const std::set<NeighborEntry>& activeNeighbors
    ) const;

    // Evaluates if the local node can accept or forward a message.
    // Deterministically drops messages exceeding MAX_HOP_COUNT, MAX_PAYLOAD_SIZE_BYTES,
    // or exceeding the MAX_MESSAGES_PER_SECOND rate limit.
    bool validateRouteConstraints(
        const AmbientMeshMessage& message,
        uint32_t currentMessagesPerSecond
    ) const;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_ROUTING_ENGINE_HPP
