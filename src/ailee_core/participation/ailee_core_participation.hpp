#pragma once

#include "ambient_ai_node_identity.hpp"
#include "../../wnn/identity/wave_identity_phase_binding.hpp"

namespace ailee {
namespace core {
namespace participation {

class WnnParticipationIntegration {
public:
    // CoherenceCluster -> ParticipationProof
    ailee::identity::ParticipationProof derive_proof_from_cluster(
        const ailee::wnn::CoherenceCluster& cluster,
        const ailee::identity::IdentityPayload& boundPayload,
        const std::vector<uint8_t>& nodeSignature
    ) const;
};

} // namespace participation
} // namespace core
} // namespace ailee
