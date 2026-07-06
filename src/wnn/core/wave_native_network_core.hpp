#pragma once

namespace ailee {
namespace wnn {

struct WaveState {
    double A;
    double theta;
    double omega;
    double x_dot;
    double ts;
};

class DuffingOscillator {
public:
    void step(WaveState& state);
private:
    double sum_x_c_ = 0.0;
    double sum_v_c_ = 0.0;
    double sum_ts_c_ = 0.0;
    double sum_theta_c_ = 0.0;
};

class DistributedPLL {
public:
    void synchronize(double aggregate_anchor_error);
private:
    double local_phase_ = 0.0;
};

class MeshOrchestrator;
class CoherenceEngine;
class RoutingEngine;

} // namespace wnn
} // namespace ailee
