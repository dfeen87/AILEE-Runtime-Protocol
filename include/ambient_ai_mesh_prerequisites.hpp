#include <cstdint>
#include <string>
#include <vector>
#ifndef AMBIENT_AI_MESH_PREREQUISITES_HPP
#define AMBIENT_AI_MESH_PREREQUISITES_HPP

namespace ailee {
namespace mesh {

// Canonical Topic Layout
constexpr char TOPIC_L2_STATE_DIFF[] = "ailee/l2/state_diff";
constexpr char TOPIC_ZK_PROOF[]      = "ailee/l2/zk_proof";
constexpr char TOPIC_TELEMETRY[]     = "ailee/ai/telemetry";
constexpr char TOPIC_EPOCH_ANCHOR[]  = "ailee/l2/epoch_anchor";

// Communication rules:
// 1. All payloads must be strictly binary length-prefixed format (JSON prohibited in mesh payloads).
// 2. Nodes validate the ECDSA signature of the sender via PeerId before routing.
// 3. Rate limiting is driven by `ReputationRateLimiter` using a monotonic logical clock, never wall-clock time.

class IMeshTransport {
public:
    virtual ~IMeshTransport() = default;

    // Broadcast a fully serialized epoch proposal or Diff
    virtual void publish(const std::string& topic, const std::vector<uint8_t>& binaryPayload) = 0;
};

} // namespace mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_PREREQUISITES_HPP
