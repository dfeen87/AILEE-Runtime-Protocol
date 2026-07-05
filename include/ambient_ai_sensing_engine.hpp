#ifndef AMBIENT_AI_SENSING_ENGINE_HPP
#define AMBIENT_AI_SENSING_ENGINE_HPP

#include <cstdint>
#include <vector>

namespace ailee {
namespace ambient_node {

// Deterministic bucketing enums for privacy preservation
enum class PowerState : uint8_t { LOW = 0, MEDIUM, HIGH };
enum class LocationClass : uint8_t { UNKNOWN = 0, INDOOR, URBAN_ZONE, RURAL_ZONE };
enum class ConnectivityBand : uint8_t { POOR = 0, FAIR, GOOD, EXCELLENT };
enum class TimeOfDayBand : uint8_t { MORNING = 0, AFTERNOON, EVENING, NIGHT };

// A strictly sanitized, privacy-preserved snapshot of device state.
struct AmbientSignalBucket {
    PowerState power;
    LocationClass location;
    ConnectivityBand connectivity;
    TimeOfDayBand timeOfDay;

    // Serialization must be stable across all architectures
    std::vector<uint8_t> serialize() const;
};

struct AmbientSignalSnapshot {
    uint64_t logicalTimestamp; // Protocol-defined tick
    AmbientSignalBucket bucket;

    // Hash based on bucket state + timestamp
    std::vector<uint8_t> hash() const;
};

class AmbientSensingEngine {
public:
    // Takes a deterministic snapshot of discretized signals based on sampling cadence
    AmbientSignalSnapshot sample() const;

    // Aggregates snapshots over a defined window (e.g., 10 minutes)
    std::vector<AmbientSignalSnapshot> getAggregatedWindow() const;

    // Prunes snapshots older than the configured retention window
    void pruneOldSnapshots(uint64_t currentLogicalTimestamp);
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_SENSING_ENGINE_HPP
