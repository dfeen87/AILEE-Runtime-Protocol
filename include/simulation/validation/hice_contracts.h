#pragma once

namespace ailee {
namespace simulation {
namespace validation {

struct HiceMetrics {
    double covariance_error;
    double spectral_drift;
    double delta_memory;
    double context_leakage;
    double null_matching_rate;
    double delta_auc;
    double ci_lower_bound;
    double ci_point_estimate;
};

struct HiceThresholds {
    double max_covariance_error;
    double max_spectral_drift;
    double min_delta_memory;
    double max_context_leakage;
    double min_null_matching_rate;
    double min_delta_auc;

    // Default V27 tolerances
    static HiceThresholds v27_defaults() {
        return {
            1e-6,   // max_covariance_error
            1e-3,   // max_spectral_drift
            -0.05,  // min_delta_memory (meaning memory loss shouldn't exceed -0.05)
            0.01,   // max_context_leakage
            0.95,   // min_null_matching_rate (within 5% tolerance)
            0.03    // min_delta_auc
        };
    }
};

struct HiceResult {
    bool hice2_ok;
    bool hice3_ok;
    bool hice4_ok;
    bool hice5_ok;
    bool hice6_ok;
    bool hice7_ok;
    bool practical_margin_ok;

    bool all_ok() const {
        return hice2_ok && hice3_ok && hice4_ok && hice5_ok && hice6_ok && hice7_ok && practical_margin_ok;
    }
};

class HiceValidator {
public:
    static HiceResult evaluate_hice_contracts(
        const HiceMetrics& metrics,
        const HiceThresholds& thresholds,
        double practical_margin
    );

private:
    static bool check_practical_margin_ci(double ci_lower_bound, double practical_margin);
};

} // namespace validation
} // namespace simulation
} // namespace ailee
