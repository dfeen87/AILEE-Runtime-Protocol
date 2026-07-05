#ifndef AMBIENT_AI_SUBSYSTEM_INTEGRATION_HPP
#define AMBIENT_AI_SUBSYSTEM_INTEGRATION_HPP

#include <memory>
#include <vector>

// Explicit integration points across the 5 canonical substrates
namespace ailee {
namespace ambient_node { class AmbientNodeModeController; }
namespace ambient_mesh { class IMeshP2PBridge; }
namespace identity { struct ParticipationProof; }
namespace energy { struct EnergyProfile; }
namespace consensus { class AmbientConsensusStateMachine; }
}

namespace ailee {
namespace runtime {

// Represents a deterministic binding rule between two disjoint systems
struct AmbientSubsystemBinding {
    bool isNodeModeEnabled;
    bool isMeshEnabled;
    bool isConsensusActive;

    // Checks bounds and constraints, e.g., low-power compatibility
    bool validateBindingInvariants() const;
};

class AmbientSubsystemContext {
public:
    // Node mode -> mesh logic binding
    void bridgeNodeModeToMesh(
        ambient_node::AmbientNodeModeController& nodeMode,
        ambient_mesh::IMeshP2PBridge& meshBridge
    );

    // Mesh -> participation proof accumulation
    void aggregateMeshParticipation(
        const ambient_mesh::IMeshP2PBridge& meshBridge,
        std::vector<identity::ParticipationProof>& proofs
    );

    // Participation -> energy ledger calculation
    void computeEnergyFromParticipation(
        const std::vector<identity::ParticipationProof>& proofs,
        std::vector<energy::EnergyProfile>& profiles
    );

    // Energy -> L2 consensus module injection
    void feedEnergyToConsensus(
        const std::vector<energy::EnergyProfile>& profiles,
        consensus::AmbientConsensusStateMachine& consensusMachine
    );

    // Final L2 consensus outputs bind to anchoring via the anchoring pipeline
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_SUBSYSTEM_INTEGRATION_HPP
