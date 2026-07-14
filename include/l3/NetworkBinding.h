#pragma once

#include <string>
#include <vector>

#include "protocol/ProtocolFrame.hpp"

struct RemotePeerRoute {
    std::string address;
};

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
