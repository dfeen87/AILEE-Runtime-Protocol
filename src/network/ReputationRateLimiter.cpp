// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
#include "ReputationRateLimiter.h"
#include "LogicalClock.h"
#include <algorithm>

namespace ailee::network {

ReputationRateLimiter::ReputationRateLimiter(const RateLimiterConfig& config)
    : config_(config) {
}

std::size_t ReputationRateLimiter::hashPayload(const std::vector<std::uint8_t>& payload) const {
    std::size_t h = 0;
    for (std::uint8_t b : payload) {
        h = (h * 131) + b;
    }
    return h;
}

bool ReputationRateLimiter::allowMessage(const std::string& peerId, double peerReputation, const std::string& topic, const std::vector<std::uint8_t>& payload) {
    std::lock_guard<std::mutex> lock(mu_);
    auto currentTick = LogicalClock::now();

    // Determine allowed rate
    double multiplier = config_.mediumRepMultiplier;
    if (peerReputation < config_.lowRepThreshold) {
        multiplier = config_.lowRepMultiplier;
    } else if (peerReputation >= config_.highRepThreshold) {
        multiplier = config_.highRepMultiplier;
    }

    std::uint32_t limit = config_.baseMessagesPerWindow;
    auto itTopic = config_.topicLimits.find(topic);
    if (itTopic != config_.topicLimits.end()) {
        limit = itTopic->second;
    }
    limit = static_cast<std::uint32_t>(limit * multiplier);
    if (limit == 0) limit = 1; // Always allow at least 1 if not strictly 0 config

    auto& state = peerStates_[peerId];
    if (state.windowStartTick == 0 || currentTick >= state.windowStartTick + config_.windowSizeTicks) {
        state.messageCount = 0;
        state.windowStartTick = currentTick;
        state.recentPayloadHashes.clear();
    }

    if (state.messageCount >= limit) {
        return false;
    }

    std::size_t pHash = hashPayload(payload);
    if (std::find(state.recentPayloadHashes.begin(), state.recentPayloadHashes.end(), pHash) != state.recentPayloadHashes.end()) {
        // Penalize duplicate diffs by denying
        return false;
    }

    state.recentPayloadHashes.push_back(pHash);
    if (state.recentPayloadHashes.size() > 50) {
        state.recentPayloadHashes.erase(state.recentPayloadHashes.begin());
    }

    state.messageCount++;
    return true;
}

} // namespace ailee::network
