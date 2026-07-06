#include "ambient_ai_runtime_model.hpp"
#include "ambient_ai_runtime_state_machine.hpp"
#include "ambient_ai_subsystem_integration.hpp"
#include "ambient_ai_epoch.hpp"
#include "ambient_ai_runtime_anchoring_pipeline.hpp"
#include "../../include/ambient_ai_node_identity.hpp"
#include "../../include/ambient_ai_energy_model.hpp"
#include "../../include/ambient_ai_event_commitment.hpp"
#include "../../include/ambient_ai_l2_merkleization.hpp"
#include "../../include/ambient_ai_taproot_anchor.hpp"
#include <sstream>
#include <iomanip>

namespace ailee {
namespace runtime {

void AmbientRuntimeEventLoop::tick(uint64_t logicalTimestamp) {
    (void)logicalTimestamp;
}

bool AmbientRuntimeEventLoop::enqueueTask(const std::vector<uint8_t>& serializedTask) {
    (void)serializedTask;
    return false;
}

void AmbientRuntimeScheduler::executeScheduledTasks(uint64_t logicalTimestamp) {
    (void)logicalTimestamp;
}

bool AmbientRuntimeScheduler::validateTaskConstraints(const std::vector<uint8_t>& serializedTask) const {
    (void)serializedTask;
    return false;
}

AmbientRuntime::AmbientRuntime(const AmbientRuntimeConfig& config) : isRunning(false) {
    (void)config;
}

AmbientRuntime::~AmbientRuntime() {}

bool AmbientRuntime::initialize() {
    isRunning = true;
    return true;
}

void AmbientRuntime::step(uint64_t logicalTimestamp, uint64_t currentBitcoinHeight) {
    if (!isRunning) return;
    (void)logicalTimestamp;
    (void)currentBitcoinHeight;
}

AmbientRuntime::EpochResult AmbientRuntime::run_epoch(uint64_t epoch_index) {
    AmbientRuntimeAnchoringPipeline pipeline;

    // Retrieve state internally
    std::vector<ailee::identity::ParticipationProof> proofs;
    std::vector<ailee::energy::EnergyProfile> profiles;
    ailee::ambient::AmbientEventAggregator eventAggregator;

    pipeline.collectEpochOutputs(proofs, profiles, eventAggregator);
    pipeline.merkleizeOutputs();

    auto final_epoch = pipeline.produceFinalEpochRoot(epoch_index, epoch_index * 144, (epoch_index + 1) * 144);

    // Create actual Taproot Anchor to complete the anchoring step deterministically
    anchoring::TaprootAnchor anchor;
    pipeline.commitToTaproot(final_epoch, anchor);

    // Compute the actual canonical hash that becomes our L2 Epoch root.
    protocol::Hash256 final_commitment = final_epoch.computeEpochCommitment();

    // Convert to hex string
    std::stringstream ss;
    for(uint8_t byte : final_commitment) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    EpochResult res;
    res.state_root = ss.str();
    return res;
}

void AmbientRuntime::shutdown() {
    isRunning = false;
}

}
}
