#ifndef AMBIENT_AI_NODE_CONFIG_HPP
#define AMBIENT_AI_NODE_CONFIG_HPP

#include "ambient_ai_routing_engine.hpp" // For AmbientLowPowerProfile

namespace ailee {
namespace ambient_node {

struct AmbientNodeConfig {
    bool ambientNodeModeEnabled = false; // OFF by default
    AmbientLowPowerProfile lowPowerProfile;

    // Safety thresholds
    uint32_t cpuLoadThresholdPercent = 20; // Max CPU impact
};

} // namespace ambient_node

namespace core {

// Hypothetical injection point within AILEE CORE Main Controller
class CoreController {
public:
    // Core Controller accepts the ambient config and optionally instantiates AmbientNodeModeController
    void initialize(const ambient_node::AmbientNodeConfig& ambientConfig) {
        if (ambientConfig.ambientNodeModeEnabled) {
            // instantiate ambient_node::AmbientNodeModeController
            // ...
        }
    }
};

} // namespace core
} // namespace ailee

#endif // AMBIENT_AI_NODE_CONFIG_HPP
