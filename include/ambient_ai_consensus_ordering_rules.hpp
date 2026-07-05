#include <cstdint>
#include <vector>
#ifndef AMBIENT_AI_CONSENSUS_ORDERING_RULES_HPP
#define AMBIENT_AI_CONSENSUS_ORDERING_RULES_HPP

#include <vector>
#include <string>

namespace ailee {
namespace identity {
    struct ParticipationProof;
}
namespace ambient_mesh {
    struct AmbientMeshParticipationSummary { uint32_t successfulRoutes; uint32_t failedRoutes; uint64_t meshUptimeSegments; std::vector<uint8_t> serialize() const; };
}
namespace ambient {
    struct AmbientEvent;
}
namespace energy {
    struct EnergyProfile;
}
}

namespace ailee {
namespace consensus {

struct AmbientConsensusOrderingRules {
    static void orderParticipationProofs(std::vector<identity::ParticipationProof>& proofs);

    static void orderEnergyProfiles(std::vector<energy::EnergyProfile>& profiles);

    struct LabeledMeshSummary {
        std::string peerId;
        ambient_mesh::AmbientMeshParticipationSummary summary;
    };
    static void orderMeshSummaries(std::vector<LabeledMeshSummary>& summaries);

    static void orderAmbientEvents(std::vector<ambient::AmbientEvent>& events);
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_CONSENSUS_ORDERING_RULES_HPP
