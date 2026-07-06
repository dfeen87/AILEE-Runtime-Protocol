#include "ailee_core_identity.hpp"

namespace ailee {
namespace core {
namespace identity {

ailee::identity::IdentityPayload WnnIdentityIntegration::bind_peer_to_identity(
    const ailee::wnn::PeerId& wnnPeerId,
    const ailee::identity::BuildMetadata& metadata,
    const std::string& publicKeyHex,
    uint64_t epochId
) const {
    ailee::identity::IdentityPayload payload;
    payload.peerId = std::string(wnnPeerId.begin(), wnnPeerId.end());
    payload.publicKeyHex = publicKeyHex;
    payload.metadata = metadata;
    payload.epochId = epochId;
    payload.staticAttribute = 0;
    return payload;
}

} // namespace identity
} // namespace core
} // namespace ailee
