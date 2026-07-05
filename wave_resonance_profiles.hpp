#pragma once

namespace ailee {
namespace wnn {

struct DuffingConstants {
    double omega;
    double alpha;
    double beta;
    double delta;
    double gamma;
};

struct HardwareProfile {
    double omega;
    double alpha;
    double beta;
    double delta;
};

class CalibrationMode {
public:
    void run_sweep();
};

} // namespace wnn
} // namespace ailee
