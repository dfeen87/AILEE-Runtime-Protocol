#include "wave_epoch_phase_state_machine.hpp"

namespace ailee {
namespace wnn {

double generate_phase_salt(double /*ts*/, double /*omega*/) {
    return 0.0;
}

void PllController::monitor_phase_error() {
}

} // namespace wnn
} // namespace ailee
