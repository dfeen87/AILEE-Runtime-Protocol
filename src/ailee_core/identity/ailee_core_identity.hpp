#pragma once

#include "ambient_ai_node_identity.hpp"
#include "../../wnn/identity/wave_identity_phase_binding.hpp"
#include <string>

namespace ailee {
namespace core {
namespace identity {

class WnnIdentityIntegration {
public:
    // PeerId -> IdentityPayload
    ailee::identity::IdentityPayload bind_peer_to_identity(
        const ailee::wnn::PeerId& wnnPeerId,
        const ailee::identity::BuildMetadata& metadata,
        const std::string& publicKeyHex,
        uint64_t epochId
    ) const;
};

} // namespace identity
} // namespace core
} // namespace ailee
