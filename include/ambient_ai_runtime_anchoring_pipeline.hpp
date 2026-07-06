#ifndef AMBIENT_AI_RUNTIME_ANCHORING_PIPELINE_HPP
#define AMBIENT_AI_RUNTIME_ANCHORING_PIPELINE_HPP

#include <vector>
#include <cstdint>

#include "ambient_ai_node_identity.hpp"
#include "ambient_ai_energy_model.hpp"
#include "ambient_ai_event_commitment.hpp"
#include "ambient_ai_l2_merkleization.hpp"

namespace ailee {
namespace protocol { struct AmbientAIAnchoringEpoch; }
namespace anchoring { class TaprootAnchor; }
}

namespace ailee {
namespace runtime {

class AmbientRuntimeAnchoringPipeline {
public:
    // Collects outputs from the current executed epoch
    void collectEpochOutputs(
        const std::vector<identity::ParticipationProof>& proofs,
        const std::vector<energy::EnergyProfile>& profiles,
        const ambient::AmbientEventAggregator& eventAggregator
    );

    // Performs canonical merkleization of collected outputs strictly using libsecp256k1
    void merkleizeOutputs();

    // Generates the final L2 epoch structure
    protocol::AmbientAIAnchoringEpoch produceFinalEpochRoot(
        uint64_t epochId,
        uint64_t startHeight,
        uint64_t endHeight
    ) const;

    // Commits the epoch root into the Bitcoin Taproot Anchor abstraction
    void commitToTaproot(
        const protocol::AmbientAIAnchoringEpoch& epoch,
        anchoring::TaprootAnchor& anchor
    ) const;

private:
    std::vector<identity::ParticipationProof> collected_proofs_;
    std::vector<energy::EnergyProfile> collected_profiles_;

    consensus::Hash256 cached_participation_root_ = {0};
    consensus::Hash256 cached_energy_root_ = {0};
    consensus::Hash256 cached_event_root_ = {0};
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_RUNTIME_ANCHORING_PIPELINE_HPP
