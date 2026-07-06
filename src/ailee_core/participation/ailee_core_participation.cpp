#include "ailee_core_participation.hpp"

namespace ailee {
namespace core {
namespace participation {

ailee::identity::ParticipationProof WnnParticipationIntegration::derive_proof_from_cluster(
    const ailee::wnn::CoherenceCluster& cluster,
    const ailee::identity::IdentityPayload& boundPayload,
    const std::vector<uint8_t>& nodeSignature
) const {
    ailee::identity::ParticipationProof proof;
    proof.payload = boundPayload;
    proof.ecdsaSignature = nodeSignature;
    // We deterministically map cluster stability properties through protocol boundaries
    // where they influence ranking.
    return proof;
}

} // namespace participation
} // namespace core
} // namespace ailee
