#pragma once

#include <cstdint>

namespace ailee {
namespace wnn {

constexpr double SCHMITT_LOCK_IN_THRESHOLD = 0.75;
constexpr double SCHMITT_DROP_OUT_THRESHOLD = 0.65;
constexpr double EPOCH_WINDOW_SECONDS = 2.5;

double generate_phase_salt(double ts, double omega);

class PllController {
public:
    void monitor_phase_error(double incoming_phase_error, double current_ts, double current_omega, double incoming_salt);
private:
    bool is_replay_detected_ = false;
    double ema_score_ = 1.0;
    bool is_active_ = false;
};

} // namespace wnn
} // namespace ailee

namespace ailee {
namespace wnn {
class PhaseStateMachine {
public:
    double get_phase_error() const;
    bool is_locked() const;
private:
    bool locked_ = true;
    double error_ = 0.0;
};
}
}
