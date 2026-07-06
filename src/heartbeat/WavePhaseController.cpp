#include "WavePhaseController.hpp"
#include <cmath>

namespace ailee {
namespace heartbeat {

WavePhaseController::WavePhaseController(ailee::wnn::WaveState& wave_state)
    : wave_state_(wave_state), previous_phase_(wave_state.theta) {}

bool WavePhaseController::detect_rollover() {
    double current_phase = wave_state_.theta;
    bool rolled_over = (current_phase < previous_phase_);
    previous_phase_ = current_phase;
    return rolled_over;
}

} // namespace heartbeat
} // namespace ailee
