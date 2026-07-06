#include "ambient_ai_runtime_anchoring_pipeline.hpp"
#include "../../include/ambient_ai_taproot_anchor.hpp"
#include "../../include/ambient_ai_epoch.hpp"

namespace ailee {
namespace runtime {

void AmbientRuntimeAnchoringPipeline::collectEpochOutputs(
    const std::vector<ailee::identity::ParticipationProof>& proofs,
    const std::vector<ailee::energy::EnergyProfile>& profiles,
    const ailee::ambient::AmbientEventAggregator& eventAggregator
) {
    collected_proofs_ = proofs;
    collected_profiles_ = profiles;
    cached_event_root_ = eventAggregator.finalizeEpochRoot();
}

void AmbientRuntimeAnchoringPipeline::merkleizeOutputs() {
    ailee::consensus::AmbientL2Merkleizer merkleizer;

    std::vector<ailee::consensus::Hash256> proof_hashes;
    for (const auto& proof : collected_proofs_) {
        proof_hashes.push_back(proof.payload.hash());
    }
    cached_participation_root_ = merkleizer.computeMerkleRoot(proof_hashes);

    std::vector<ailee::consensus::Hash256> energy_hashes;
    for (const auto& profile : collected_profiles_) {
        energy_hashes.push_back(profile.hash());
    }
    cached_energy_root_ = merkleizer.computeMerkleRoot(energy_hashes);
}

protocol::AmbientAIAnchoringEpoch AmbientRuntimeAnchoringPipeline::produceFinalEpochRoot(
    uint64_t epochId,
    uint64_t startHeight,
    uint64_t endHeight
) const {
    ailee::consensus::AmbientL2Merkleizer merkleizer;

    ailee::consensus::Hash256 l2StateRoot = {0};
    l2StateRoot[0] = static_cast<uint8_t>(epochId & 0xFF);
    l2StateRoot[1] = static_cast<uint8_t>((epochId >> 8) & 0xFF);

    return merkleizer.buildEpochCommitment(
        epochId,
        startHeight,
        endHeight,
        l2StateRoot,
        cached_energy_root_,
        cached_participation_root_,
        cached_event_root_
    );
}

void AmbientRuntimeAnchoringPipeline::commitToTaproot(
    const protocol::AmbientAIAnchoringEpoch& epoch,
    anchoring::TaprootAnchor& anchor
) const {
    anchor.buildEpochCommitment(epoch);
}

} // namespace runtime
} // namespace ailee
