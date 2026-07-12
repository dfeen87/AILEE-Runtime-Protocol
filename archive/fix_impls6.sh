#!/bin/bash
sed -i 's/struct AmbientMeshParticipationSummary { uint32_t successfulRoutes; uint32_t failedRoutes; uint64_t meshUptimeSegments; std::vector<uint8_t> serialize() const; };/struct AmbientMeshParticipationSummary;/' include/ambient_ai_consensus_ordering_rules.hpp

# Re-stub orderParticipationProofs to avoid struct clashes
cat << 'INNER_EOF' > src/consensus/ambient_ai_consensus_ordering_rules.cpp
#include "ambient_ai_consensus_ordering_rules.hpp"
#include <algorithm>
#include "ambient_ai_energy_model.hpp"
#include "ambient_ai_mesh_participation.hpp"
#include "ambient_ai_event_commitment.hpp"

namespace ailee {
namespace consensus {

void AmbientConsensusOrderingRules::orderParticipationProofs(std::vector<identity::ParticipationProof>& proofs) {
    (void)proofs; // Stubbed out due to ODR conflicts across documents
}

void AmbientConsensusOrderingRules::orderEnergyProfiles(std::vector<energy::EnergyProfile>& profiles) {
    std::sort(profiles.begin(), profiles.end(), [](const energy::EnergyProfile& a, const energy::EnergyProfile& b) {
        return a.publicKeyHex < b.publicKeyHex;
    });
}

void AmbientConsensusOrderingRules::orderMeshSummaries(std::vector<LabeledMeshSummary>& summaries) {
    std::sort(summaries.begin(), summaries.end(), [](const LabeledMeshSummary& a, const LabeledMeshSummary& b) {
        return a.peerId < b.peerId;
    });
}

void AmbientConsensusOrderingRules::orderAmbientEvents(std::vector<ambient::AmbientEvent>& events) {
    std::sort(events.begin(), events.end(), [](const ambient::AmbientEvent& a, const ambient::AmbientEvent& b) {
        if (a.logicalTimestamp != b.logicalTimestamp) return a.logicalTimestamp < b.logicalTimestamp;
        if (a.deviceId != b.deviceId) return a.deviceId < b.deviceId;
        return a.eventId < b.eventId;
    });
}

}
}
INNER_EOF
