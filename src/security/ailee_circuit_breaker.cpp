/**
 * ailee_circuit_breaker.cpp
 *
 * Implements the AILEE Autonomous Circuit Breaker (canonical v1.4)
 *
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#include "ailee_circuit_breaker.h"

#include <iostream>
#include <sstream>

#include "ailee_recovery_protocol.h"

namespace ailee {

namespace {
    // Minimum blocks between state transitions to prevent oscillation
    const uint64_t TRANSITION_HYSTERESIS_BLOCKS = 10;

    // Helper logger (replaceable with your structured logger)
    void localLog(uint64_t blockHeight, const std::string& level, const std::string& msg) {
        std::ostringstream ss;
        ss << "[Block " << blockHeight << "] [" << level << "] " << msg;
        std::cerr << ss.str() << std::endl;
    }

    // Rate-limited transition helper
    bool allowTransition(BreakerState& state, uint64_t currentHeight) {
        if (currentHeight >= state.lastTransitionHeight &&
            (currentHeight - state.lastTransitionHeight) < TRANSITION_HYSTERESIS_BLOCKS) {
            return false;
        }
        state.lastTransitionHeight = currentHeight;
        return true;
    }

    // Safe wrapper to attempt RecoveryProtocol recording (if available)
    void recordIncidentSafe(uint64_t blockHeight, const std::string& key, const std::string& detail) {
        try {
            ailee::RecoveryProtocol::recordIncident(key, detail);
        } catch (...) {
            // Swallow exceptions to avoid cascading failures in breaker logic
            localLog(blockHeight, "WARN", std::string("RecoveryProtocol record failed for: ") + key);
        }
    }
} // namespace (internal)

uint64_t CircuitBreaker::computeAIDrift(uint64_t targetBlockSizeScaled, uint64_t proposedBlockSizeScaled) {
    if (targetBlockSizeScaled == 0) return 0;

    uint64_t diff = proposedBlockSizeScaled > targetBlockSizeScaled ?
                    proposedBlockSizeScaled - targetBlockSizeScaled :
                    targetBlockSizeScaled - proposedBlockSizeScaled;

    return (diff * CB_SCALE) / targetBlockSizeScaled;
}

BreakerReport CircuitBreaker::monitor(
    BreakerState& state,
    uint64_t blockHeight,
    uint64_t proposedBlockSizeScaled,
    uint64_t currentLatencyScaled,
    int peerCount,
    uint64_t targetBlockSizeScaled,
    uint64_t currentEISScaled,
    uint64_t sensorConfidenceScaled,
    uint64_t previousEISScaled
) {
    BreakerReport report;
    report.state = SystemState::OPTIMIZED;
    report.reason = "Unknown";
    report.eisScaled = currentEISScaled;
    report.entropyDeltaScaled = currentEISScaled > previousEISScaled ?
                                currentEISScaled - previousEISScaled :
                                previousEISScaled - currentEISScaled;
    report.driftScoreScaled = computeAIDrift(targetBlockSizeScaled, proposedBlockSizeScaled);

    // --------------- Hard Red-lines ----------------
    if (proposedBlockSizeScaled > MAX_SAFE_BLOCK_SIZE_SCALED) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Unsafe block size proposal — exceeds MAX_SAFE_BLOCK_SIZE_MB.";
        localLog(blockHeight, "ALERT", report.reason + " proposed=" + std::to_string(proposedBlockSizeScaled));
        recordIncidentSafe(blockHeight, "CircuitBreaker_HardBlockSize", report.reason);
        // Update last state
        state.lastState = report.state;
        return report;
    }

    if (currentLatencyScaled > MAX_LATENCY_TOLERANCE_SCALED) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Network latency exceeds tolerance.";
        localLog(blockHeight, "ALERT", report.reason + " latency=" + std::to_string(currentLatencyScaled));
        recordIncidentSafe(blockHeight, "CircuitBreaker_HighLatency", report.reason);
        state.lastState = report.state;
        return report;
    }

    if (peerCount < MIN_PEER_COUNT) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Peer count below safe minimum.";
        localLog(blockHeight, "ALERT", report.reason + " peers=" + std::to_string(peerCount));
        recordIncidentSafe(blockHeight, "CircuitBreaker_LowPeers", report.reason);
        state.lastState = report.state;
        return report;
    }

    // --------------- Entropy Surge Detection ----------------
    if (report.entropyDeltaScaled > MAX_ENTROPY_SURGE_DELTA_SCALED) {
        // Soft trip: throttle AI but do not fully disable it
        report.state = SystemState::SOFT_TRIP;
        report.reason = "Entropy surge detected — EIS changed rapidly.";
        localLog(blockHeight, "WARN", report.reason + " delta=" + std::to_string(report.entropyDeltaScaled));
        recordIncidentSafe(blockHeight, "CircuitBreaker_EntropySurge", "delta=" + std::to_string(report.entropyDeltaScaled));

        // Apply hysteresis: only transition if allowed
        if (!allowTransition(state, blockHeight)) {
            localLog(blockHeight, "DEBUG", "Entropy surge detected but transition suppressed by hysteresis.");
            // keep previous state if transition suppressed
            report.state = state.lastState;
            report.reason += " (transition suppressed)";
        } else {
            state.lastState = report.state;
        }
        return report;
    }

    // --------------- EIS Floor Check ----------------
    if (currentEISScaled < MIN_EIS_FOR_OPTIMIZATION_SCALED) {
        // Low EIS: soft trip first. If repeatedly low, escalate to SAFE_MODE.
        report.state = SystemState::SOFT_TRIP;
        report.reason = "Energy Integrity Score below MIN_EIS_FOR_OPTIMIZATION.";
        localLog(blockHeight, "WARN", report.reason + " EIS=" + std::to_string(currentEISScaled));
        recordIncidentSafe(blockHeight, "CircuitBreaker_LowEIS", "eis=" + std::to_string(currentEISScaled));

        // If previous state was already SOFT_TRIP and transition allowed -> escalate
        if (state.lastState == SystemState::SOFT_TRIP && allowTransition(state, blockHeight)) {
            report.state = SystemState::SAFE_MODE;
            report.reason = "Persistent low EIS — escalating to SAFE_MODE.";
            localLog(blockHeight, "ALERT", report.reason);
            recordIncidentSafe(blockHeight, "CircuitBreaker_EscalateLowEIS", report.reason);
            state.lastState = report.state;
        } else {
            state.lastState = report.state;
        }

        return report;
    }

    // --------------- AI Drift Monitoring ----------------
    if (report.driftScoreScaled > MAX_AI_DRIFT_SCORE_SCALED) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "AI drift exceeds MAX_AI_DRIFT_SCORE — reverting to safe defaults.";
        localLog(blockHeight, "ALERT", report.reason + " drift=" + std::to_string(report.driftScoreScaled));
        recordIncidentSafe(blockHeight, "CircuitBreaker_AIDrift", "drift=" + std::to_string(report.driftScoreScaled));
        state.lastState = report.state;
        return report;
    }

    // --------------- Critical checks (aggregate risk) ----------------
    // If multiple soft signals present, escalate to SAFE_MODE
    int softSignals = 0;
    if (report.entropyDeltaScaled > (MAX_ENTROPY_SURGE_DELTA_SCALED * 6) / 10) softSignals++;
    if (sensorConfidenceScaled < 2500) softSignals++; // 0.25 * 10000
    if (currentLatencyScaled > (MAX_LATENCY_TOLERANCE_SCALED * 75) / 100) softSignals++;
    if (softSignals >= 3) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Multiple concurrent soft signals — escalating to SAFE_MODE.";
        localLog(blockHeight, "ALERT", report.reason);
        recordIncidentSafe(blockHeight, "CircuitBreaker_MultiSoftSignals", report.reason);
        state.lastState = report.state;
        return report;
    }

    // --------------- All checks passed: OPTIMIZED ----------------
    report.state = SystemState::OPTIMIZED;
    report.reason = "All safety checks passed — AI optimization permitted.";
    localLog(blockHeight, "INFO", report.reason);
    state.lastState = report.state;

    return report;
}

} // namespace ailee
