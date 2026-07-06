#include "HeartbeatOrchestrator.hpp"

namespace ailee {
namespace heartbeat {

HeartbeatOrchestrator::HeartbeatOrchestrator(
    WavePhaseController& phase_controller,
    EpochScheduler& epoch_scheduler,
    DeterministicEpochExecutorWrapper& epoch_executor,
    IBitcoinAnchor& bitcoin_anchor
) : phase_controller_(phase_controller),
    epoch_scheduler_(epoch_scheduler),
    epoch_executor_(epoch_executor),
    bitcoin_anchor_(bitcoin_anchor) {}

void HeartbeatOrchestrator::step() {
    if (phase_controller_.detect_rollover()) {
        std::cout << "[HeartbeatOrchestrator] Wave-phase rollover detected. Triggering heartbeat event." << std::endl;

        // 1. Advance the epoch counter deterministically
        epoch_scheduler_.advance_epoch();
        uint64_t current_epoch = epoch_scheduler_.get_current_epoch();
        std::cout << "[HeartbeatOrchestrator] Advanced to epoch: " << current_epoch << std::endl;

        // 2. Execute the next epoch using the existing AILEE Core deterministic runtime
        std::string state_root = epoch_executor_.run_epoch(current_epoch);
        std::cout << "[HeartbeatOrchestrator] Epoch execution complete. State root: " << state_root << std::endl;

        // 3. Anchor the new state root to Bitcoin
        AnchorResult anchor_res = bitcoin_anchor_.anchor_state_root(current_epoch, state_root);
        std::cout << "[HeartbeatOrchestrator] Anchored state root. Commitment hash: " << anchor_res.commitment_hash << std::endl;
    }
}

} // namespace heartbeat
} // namespace ailee
