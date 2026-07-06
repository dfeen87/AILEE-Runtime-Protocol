#ifndef AILEE_HEARTBEAT_ORCHESTRATOR_HPP
#define AILEE_HEARTBEAT_ORCHESTRATOR_HPP

#include "WavePhaseController.hpp"
#include "EpochScheduler.hpp"
#include "DeterministicEpochExecutorWrapper.hpp"
#include "BitcoinAnchoringInterface.hpp"
#include <iostream>

namespace ailee {
namespace heartbeat {

class HeartbeatOrchestrator {
public:
    HeartbeatOrchestrator(
        WavePhaseController& phase_controller,
        EpochScheduler& epoch_scheduler,
        DeterministicEpochExecutorWrapper& epoch_executor,
        IBitcoinAnchor& bitcoin_anchor
    );

    // Steps the orchestrator. If a phase rollover is detected, triggers heartbeat.
    void step();

private:
    WavePhaseController& phase_controller_;
    EpochScheduler& epoch_scheduler_;
    DeterministicEpochExecutorWrapper& epoch_executor_;
    IBitcoinAnchor& bitcoin_anchor_;
};

} // namespace heartbeat
} // namespace ailee

#endif // AILEE_HEARTBEAT_ORCHESTRATOR_HPP
