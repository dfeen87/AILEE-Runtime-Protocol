#include "wave_epoch_phase_state_machine.hpp"
#include <cmath>
#include <cstring>

namespace ailee {
namespace wnn {

// Deterministic FNV-1a hash function for 64-bit integers
inline uint64_t fnv1a_64(uint64_t val) {
    uint64_t hash = 14695981039346656037ULL;
    for (int i = 0; i < 8; ++i) {
        uint8_t byte = (val >> (i * 8)) & 0xFF;
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

double generate_phase_salt(double ts, double omega) {
    uint64_t epoch = static_cast<uint64_t>(std::floor(ts / EPOCH_WINDOW_SECONDS));
    uint64_t w_bits;
    double omega_d = omega;
    std::memcpy(&w_bits, &omega_d, sizeof(omega_d));

    uint64_t combined = epoch ^ w_bits;
    uint64_t hash = fnv1a_64(combined);

    return (hash % 1000) * 1e-5;
}

void PllController::monitor_phase_error(double incoming_phase_error, double current_ts, double current_omega, double incoming_salt) {
    double structural_lock_in_score = 1.0 - std::abs(incoming_phase_error * 100.0);
    if (structural_lock_in_score < 0.0) structural_lock_in_score = 0.0;
    if (structural_lock_in_score > 1.0) structural_lock_in_score = 1.0;

    double alpha = 0.1;
    ema_score_ = alpha * structural_lock_in_score + (1.0 - alpha) * ema_score_;

    if (!is_active_ && ema_score_ >= SCHMITT_LOCK_IN_THRESHOLD) {
        is_active_ = true;
    } else if (is_active_ && ema_score_ < SCHMITT_DROP_OUT_THRESHOLD) {
        is_active_ = false;
    }

    // Deterministic replay protection
    double expected_salt = generate_phase_salt(current_ts, current_omega);

    if (std::abs(incoming_salt - expected_salt) > 1e-7) {
        is_replay_detected_ = true;
        ema_score_ = 0.0;
        is_active_ = false;
    } else {
        is_replay_detected_ = false;
    }
}

} // namespace wnn
} // namespace ailee

namespace ailee {
namespace wnn {
double PhaseStateMachine::get_phase_error() const { return error_; }
bool PhaseStateMachine::is_locked() const { return locked_; }
}
}
