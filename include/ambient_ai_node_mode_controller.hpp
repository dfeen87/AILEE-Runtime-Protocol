#include "ambient_ai_node_config.hpp"
#ifndef AMBIENT_AI_NODE_MODE_CONTROLLER_HPP
#define AMBIENT_AI_NODE_MODE_CONTROLLER_HPP

#include <memory>
#include <cstdint>

namespace ailee {
namespace ambient_node {

// Forward declarations
class AmbientSensingEngine;
class AmbientRoutingEngine;
class AmbientL2Bridge;
class AmbientParticipationTracker;
struct AmbientNodeConfig;

// The main controller managing the lifecycle of Ambient Node Mode.
class AmbientNodeModeController {
public:
    explicit AmbientNodeModeController(const AmbientNodeConfig& config);
    ~AmbientNodeModeController();

    // Hooks directly into AILEE CORE startup.
    // Fails fast if invariants (e.g., BuildInfo metadata mismatch) fail.
    bool initialize();

    // Hooks directly into AILEE CORE shutdown.
    void shutdown();

    // Exposes current state for graceful degradation checks by the main node.
    bool isOperatingNominally() const;

private:
    std::unique_ptr<AmbientSensingEngine> sensingEngine;
    std::unique_ptr<AmbientRoutingEngine> routingEngine;
    std::unique_ptr<AmbientL2Bridge> l2Bridge;
    std::unique_ptr<AmbientParticipationTracker> participationTracker;

    AmbientNodeConfig config;
    bool isRunning;
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_NODE_MODE_CONTROLLER_HPP
