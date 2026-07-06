#ifndef AILEE_HEARTBEAT_WAVE_PHASE_CONTROLLER_HPP
#define AILEE_HEARTBEAT_WAVE_PHASE_CONTROLLER_HPP

#include "../../src/wnn/core/wave_native_network_core.hpp"

namespace ailee {
namespace heartbeat {

class WavePhaseController {
public:
    explicit WavePhaseController(ailee::wnn::WaveState& wave_state);

    bool detect_rollover();

private:
    ailee::wnn::WaveState& wave_state_;
    double previous_phase_;
};

} // namespace heartbeat
} // namespace ailee

#endif // AILEE_HEARTBEAT_WAVE_PHASE_CONTROLLER_HPP
