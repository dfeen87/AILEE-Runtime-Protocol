#pragma once

#include <string>
#include <vector>
#include "protocol/ProtocolFrame.hpp"

namespace ailee {
namespace l3 {

// Forward declarations for network snapshots
struct NetworkBlockSnapshot;
struct NetworkMempoolSnapshot;
struct NetworkPeerSnapshot;

// Forward declarations for bind_network_* result types
namespace reflection {
    struct ReflectionSnapshot;
}

namespace l1 {
    struct SettlementIngestion;
}

namespace mesh {
    struct MeshCoherenceResult;
}

// ---------------------------------------------------------
// RemotePeerRoute
// ---------------------------------------------------------
struct RemotePeerRoute {
    std::string address;
};

// ---------------------------------------------------------
// NetworkBinding
// ---------------------------------------------------------
class NetworkBinding {
public:
    // Local node identity
    std::string localNodeId() const;

    // Serialize a ProtocolFrame for transport
    std::string serializeFrame(const ProtocolFrame& pf) const;

    // Local broadcast (existing behavior)
    void broadcast(const std::string& data);

    // Mainnet broadcast to specific address
    void broadcastTo(const std::string& address, const std::string& data);

    // Routing helpers
    void registerPeerAddress(const std::string& peerId,
                             const std::string& address);
    std::string getPeerAddress(const std::string& peerId) const;

private:
    std::string local_id_;
    // In a fuller system, this would be a map<peerId, route>
};

// ---------------------------------------------------------
// bind_network_* function declarations
// ---------------------------------------------------------

reflection::ReflectionSnapshot
bind_network_block(const NetworkBlockSnapshot& net_block);

l1::SettlementIngestion
bind_network_mempool(const NetworkMempoolSnapshot& net_mempool,
                     const NetworkBlockSnapshot& net_block);

mesh::MeshCoherenceResult
bind_network_peer(const NetworkPeerSnapshot& net_peer);

} // namespace l3
} // namespace ailee
