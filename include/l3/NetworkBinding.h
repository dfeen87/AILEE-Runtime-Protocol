#pragma once

#include <string>
#include <vector>

#include "protocol/ProtocolFrame.hpp"

// Forward declarations for bind_network_* functions
namespace reflection {
    struct ReflectionSnapshot;
}

namespace l1 {
    struct SettlementIngestion;
}

namespace mesh {
    struct MeshCoherenceResult;
}

// Forward declarations for network snapshots
struct NetworkBlockSnapshot;
struct NetworkMempoolSnapshot;
struct NetworkPeerSnapshot;

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
