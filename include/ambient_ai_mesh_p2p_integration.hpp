#ifndef AMBIENT_AI_MESH_P2P_INTEGRATION_HPP
#define AMBIENT_AI_MESH_P2P_INTEGRATION_HPP

#include <cstdint>
#include <vector>
#include <string>
#include "ambient_ai_mesh_protocol.hpp" // Includes AmbientMeshLimits

namespace ailee {
namespace ambient_mesh {

// Injected configuration strictly controlling mesh behavior and limits
struct AmbientMeshConfig {
    bool ambientMeshEnabled = false; // Opt-in feature
    uint32_t maxMessagesPerSecond = AmbientMeshLimits::MAX_MESSAGES_PER_SECOND;
    uint32_t maxPayloadSizeBytes = AmbientMeshLimits::MAX_PAYLOAD_SIZE_BYTES;
};

// Decouples the deterministic semantic mesh from the underlying networking transport.
class IMeshP2PBridge {
public:
    virtual ~IMeshP2PBridge() = default;

    // Registers the deterministic mesh topic (e.g., "ailee/l2/ambient_mesh") via Gossipsub.
    virtual void registerTopics() = 0;

    // Broadcasts an AmbientMeshMessage directly to the topic.
    virtual void broadcast(const std::string& topic, const std::vector<uint8_t>& binaryPayload) = 0;

    // Callback invoked by the underlying P2P transport when a message is received on the topic.
    virtual void onMessageReceived(const std::string& peerId, const std::vector<uint8_t>& binaryPayload) = 0;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_P2P_INTEGRATION_HPP
