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
};

class DistributedPLL {
public:
    void synchronize();
};

class MeshOrchestrator;
class CoherenceEngine;
class RoutingEngine;

} // namespace wnn
} // namespace ailee
