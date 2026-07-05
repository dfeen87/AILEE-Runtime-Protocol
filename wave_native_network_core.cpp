#include "wave_native_network_core.hpp"
#include "wave_epoch_phase_state_machine.hpp"
#include <cmath>

namespace ailee {
namespace wnn {

void DuffingOscillator::step(WaveState& state) {
    // Default Duffing Constants
    double delta_c = 0.1;
    double alpha_c = -1.0;
    double beta_c = 1.0;
    double gamma_c = 0.3;

    // 1 microsecond precision
    double dt = 1e-6;

    double omega_active = state.omega + generate_phase_salt(state.ts, state.omega);

    auto f_v = [delta_c, alpha_c, beta_c, gamma_c, omega_active](double t, double x, double v) {
        return gamma_c * std::cos(omega_active * t) - delta_c * v - alpha_c * x - beta_c * x * x * x;
    };

    double k1_x = state.x_dot;
    double k1_v = f_v(state.ts, state.A, state.x_dot);

    double k2_x = state.x_dot + 0.5 * dt * k1_v;
    double k2_v = f_v(state.ts + 0.5 * dt, state.A + 0.5 * dt * k1_x, state.x_dot + 0.5 * dt * k1_v);

    double k3_x = state.x_dot + 0.5 * dt * k2_v;
    double k3_v = f_v(state.ts + 0.5 * dt, state.A + 0.5 * dt * k2_x, state.x_dot + 0.5 * dt * k2_v);

    double k4_x = state.x_dot + dt * k3_v;
    double k4_v = f_v(state.ts + dt, state.A + dt * k3_x, state.x_dot + dt * k3_v);

    double dx = (dt / 6.0) * (k1_x + 2.0 * k2_x + 2.0 * k3_x + k4_x);
    double dv = (dt / 6.0) * (k1_v + 2.0 * k2_v + 2.0 * k3_v + k4_v);

    // Kahan summation for state.A (x) using class member state
    double y_x = dx - sum_x_c_;
    double t_x = state.A + y_x;
    sum_x_c_ = (t_x - state.A) - y_x;
    state.A = t_x;

    // Kahan summation for state.x_dot (v)
    double y_v = dv - sum_v_c_;
    double t_v = state.x_dot + y_v;
    sum_v_c_ = (t_v - state.x_dot) - y_v;
    state.x_dot = t_v;

    // Kahan summation for state.ts (time)
    double y_ts = dt - sum_ts_c_;
    double t_ts = state.ts + y_ts;
    sum_ts_c_ = (t_ts - state.ts) - y_ts;
    state.ts = t_ts;

    // Kahan summation for state.theta (phase accumulator)
    double dtheta = omega_active * dt;
    double y_theta = dtheta - sum_theta_c_;
    double t_theta = state.theta + y_theta;
    sum_theta_c_ = (t_theta - state.theta) - y_theta;
    state.theta = t_theta;
}

void DistributedPLL::synchronize(double aggregate_anchor_error) {
    double k_p = 0.05; // Proportional gain

    // Adjust local phase deterministically
    double correction = k_p * aggregate_anchor_error;
    local_phase_ += correction;
}

} // namespace wnn
} // namespace ailee
