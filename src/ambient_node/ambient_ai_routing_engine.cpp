#include "ambient_ai_routing_engine.hpp"

namespace ailee {
namespace ambient_node {

AmbientRoutingEngine::AmbientRoutingEngine(const AmbientLowPowerProfile& prof) : profile(prof) {}

bool AmbientRoutingEngine::canRoute(PowerState currentPower) const {
    return static_cast<uint8_t>(currentPower) >= static_cast<uint8_t>(profile.minBatteryThreshold);
}

bool AmbientRoutingEngine::routeMessage(const std::vector<uint8_t>& message) {
    // Check max bytes per second
    if (message.size() > profile.maxBytesPerSecond) return false;
    return true;
}

void AmbientRoutingEngine::handleFailure(const std::string& peerId) {
    (void)peerId;
}

void AmbientRoutingEngine::processRetries(uint64_t logicalTimestamp) {
    (void)logicalTimestamp;
}

}
}
