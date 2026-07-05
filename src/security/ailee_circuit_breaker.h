/**
 * ailee_circuit_breaker.h
 *
 * Declarations for the AILEE Autonomous Circuit Breaker (canonical v1.4)
 *
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_CIRCUIT_BREAKER_H
#define AILEE_CIRCUIT_BREAKER_H

#include <string>

#include "ailee_energy_telemetry.h"

namespace ailee {

enum class SystemState {
    OPTIMIZED,
    SOFT_TRIP,
    SAFE_MODE,
    CRITICAL
};

constexpr uint64_t CB_SCALE = 10000;

struct BreakerReport {
    SystemState state;
    std::string reason;
    uint64_t eisScaled;
    uint64_t entropyDeltaScaled;
    uint64_t driftScoreScaled;
};

struct BreakerState {
    SystemState lastState = SystemState::OPTIMIZED;
    uint64_t lastTransitionHeight = 0;
};

// Scaled thresholds (value * CB_SCALE)
constexpr uint64_t MAX_SAFE_BLOCK_SIZE_SCALED = 40000;      // 4.0
constexpr uint64_t MAX_LATENCY_TOLERANCE_SCALED = 20000000; // 2000.0
constexpr int MIN_PEER_COUNT = 8;
constexpr uint64_t MAX_ENTROPY_SURGE_DELTA_SCALED = 3500;   // 0.35
constexpr uint64_t MIN_EIS_FOR_OPTIMIZATION_SCALED = 2000;  // 0.2
constexpr uint64_t MAX_AI_DRIFT_SCORE_SCALED = 6000;        // 0.6

class CircuitBreaker {
public:
    static uint64_t computeAIDrift(uint64_t targetBlockSizeScaled, uint64_t proposedBlockSizeScaled);
    static BreakerReport monitor(
        BreakerState& state,
        uint64_t blockHeight,
        uint64_t proposedBlockSizeScaled,
        uint64_t currentLatencyScaled,
        int peerCount,
        uint64_t targetBlockSizeScaled,
        uint64_t currentEISScaled,
        uint64_t sensorConfidenceScaled,
        uint64_t previousEISScaled
    );
};

} // namespace ailee

#endif // AILEE_CIRCUIT_BREAKER_H
