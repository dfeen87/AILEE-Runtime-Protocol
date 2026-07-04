// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <vector>

namespace ailee::network {

struct RateLimiterConfig {
    double lowRepThreshold = 0.3;
    double highRepThreshold = 0.8;

    double lowRepMultiplier = 0.25;
    double mediumRepMultiplier = 1.0;
    double highRepMultiplier = 2.0;

    std::uint32_t baseMessagesPerSecond = 10;
    std::chrono::milliseconds windowSize{1000};

    // Limits per topic
    std::unordered_map<std::string, std::uint32_t> topicLimits;
};

class ReputationRateLimiter {
public:
    explicit ReputationRateLimiter(const RateLimiterConfig& config = RateLimiterConfig{});

    // Checks if the peer is allowed to send a message on the given topic.
    // Also deduplicates identical payloads within a short window to penalize duplicate diffs.
    bool allowMessage(const std::string& peerId, double peerReputation, const std::string& topic, const std::vector<std::uint8_t>& payload);

private:
    RateLimiterConfig config_;

    struct PeerState {
        std::uint32_t messageCount = 0;
        std::chrono::steady_clock::time_point windowStart;
        std::vector<std::size_t> recentPayloadHashes;
    };

    std::unordered_map<std::string, PeerState> peerStates_;
    mutable std::mutex mu_;

    std::size_t hashPayload(const std::vector<std::uint8_t>& payload) const;
};

} // namespace ailee::network
