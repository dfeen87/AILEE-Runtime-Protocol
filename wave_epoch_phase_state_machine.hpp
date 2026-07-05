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
    void monitor_phase_error();
private:
    bool is_replay_detected_;
};

} // namespace wnn
} // namespace ailee
