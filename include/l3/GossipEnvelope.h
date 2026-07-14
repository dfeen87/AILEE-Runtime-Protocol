#pragma once

#include <cstdint>
#include "protocol/ProtocolFrame.hpp"

namespace ailee {
namespace l3 {

// ---------------------------------------------------------
// GossipEnvelope — placeholder structure
// ---------------------------------------------------------
// This is a minimal envelope used by GossipLayer and PeerSync.
// It will be expanded later when full gossip semantics are added.
// ---------------------------------------------------------

// Gossip flag constants (placeholder)
static constexpr uint32_t GOSSIP_FLAG_NONE               = 0;
static constexpr uint32_t GOSSIP_FLAG_INVALID_SIGNATURE  = 1 << 0;
static constexpr uint32_t GOSSIP_FLAG_VALID_SIGNATURE    = 1 << 1;

struct GossipEnvelope {
    bool has_protocol_frame = false;
    ProtocolFrame protocol_frame;

    uint32_t flags = GOSSIP_FLAG_NONE;

    // Placeholder validation
    bool validate() const { return true; }
};

} // namespace l3
} // namespace ailee
