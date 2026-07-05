#ifndef AMBIENT_AI_MESH_NEIGHBORHOOD_HPP
#define AMBIENT_AI_MESH_NEIGHBORHOOD_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <array>
#include "ambient_ai_mesh_protocol.hpp" // Includes NeighborEntry and Hash256

namespace ailee {
namespace ambient_mesh {

struct RecentRouteCacheEntry {
    Hash256 messageId;
    uint64_t receivedTick;
};

class LocalNeighborhoodManager {
public:
    // Adds or updates a neighbor based on observed libp2p peer exchange and energy profile.
    void updateNeighborState(
        const std::string& peerId,
        int64_t energyScore,
        uint64_t currentLogicalTick
    );

    // Enforces deterministic eviction policies based on active link state and inactivity.
    // Avoids oscillation by requiring a fixed minimum logical tick threshold before eviction.
    void pruneInactiveNeighbors(uint64_t currentLogicalTick);

    // Retrieves the deterministically sorted active neighbors.
    std::set<NeighborEntry> getActiveNeighbors() const;

    // Tracks seen messages to prevent routing loops and broadcast storms.
    // Bounded cache; entries older than a fixed tick window are evicted deterministically.
    bool observeAndCacheMessage(const Hash256& messageId, uint64_t currentLogicalTick);

private:
    std::set<NeighborEntry> neighborTable;

    // Hash string serialization to CacheEntry. std::map guarantees deterministic iteration
    // and pruning order over messageId hashes.
    std::map<std::string, RecentRouteCacheEntry> recentRouteCache;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_NEIGHBORHOOD_HPP
