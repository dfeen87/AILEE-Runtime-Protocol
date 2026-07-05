#ifndef AMBIENT_AI_ENERGY_MODEL_HPP
#define AMBIENT_AI_ENERGY_MODEL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

namespace ailee {
namespace energy {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic constants for energy calculus (Scale = 10000)
constexpr int64_t FIXED_POINT_SCALE = 10000;
constexpr int64_t ALPHA_COMPUTE     = 6000; // 0.6
constexpr int64_t BETA_UPTIME       = 3000; // 0.3
constexpr int64_t GAMMA_STORAGE     = 1000; // 0.1

// Absolute hardware bounds to prevent artificial overflow exploits
constexpr uint64_t MAX_COMPUTE_JOBS     = 1000000;      // 1M jobs per epoch
constexpr uint64_t MAX_UPTIME_MS        = 86400000;     // 24 hours in ms
constexpr uint64_t MAX_STORAGE_BYTES    = 107374182400; // 100 GB

struct EnergyProfile {
    std::string publicKeyHex; // Binds directly to IdentityPayload::publicKeyHex

    // Raw unscaled metrics accumulated deterministically during the epoch
    uint64_t computeCompletedJobs;
    uint64_t uptimeMilliseconds;
    uint64_t storageBytesProvided;

    // Deterministically scaled and combined energy value (Scale = 10000)
    int64_t calculateEpochEnergy() const {
        // Enforce protocol upper bounds
        uint64_t boundedCompute = std::min(computeCompletedJobs, MAX_COMPUTE_JOBS);
        uint64_t boundedUptime = std::min(uptimeMilliseconds, MAX_UPTIME_MS);
        uint64_t boundedStorage = std::min(storageBytesProvided, MAX_STORAGE_BYTES);

        // Use 128-bit ints for intermediate multiplication to prevent overflow
        __int128_t computeScore = static_cast<__int128_t>(boundedCompute) * ALPHA_COMPUTE;
        __int128_t uptimeScore  = static_cast<__int128_t>(boundedUptime) * BETA_UPTIME;
        __int128_t storageScore = static_cast<__int128_t>(boundedStorage) * GAMMA_STORAGE;

        __int128_t totalEnergyScaled = computeScore + uptimeScore + storageScore;

        // Downscale
        return static_cast<int64_t>(totalEnergyScaled / FIXED_POINT_SCALE);
    }

    // Serializes strictly into length-prefixed binary form for Merkle leaf creation
    std::vector<uint8_t> canonicalSerialize() const;

    Hash256 hash() const;
};

// Represents a historical cumulative state of a node's energy.
struct EnergyLedgerEntry {
    std::string publicKeyHex;
    uint64_t cumulativeEnergy; // Accrues positive energy, floors at 0 for penalties

    // Applies a deterministic epoch-over-epoch decay to prevent perpetual dominance.
    // e.g., decays by 5% every epoch: newEnergy = (cumulativeEnergy * 9500) / FIXED_POINT_SCALE
    void applyEpochDecay() {
        constexpr int64_t DECAY_FACTOR = 9500; // 0.95 multiplier
        __int128_t decayed = static_cast<__int128_t>(cumulativeEnergy) * DECAY_FACTOR;
        cumulativeEnergy = static_cast<uint64_t>(decayed / FIXED_POINT_SCALE);
    }
};

} // namespace energy
} // namespace ailee

#endif // AMBIENT_AI_ENERGY_MODEL_HPP
