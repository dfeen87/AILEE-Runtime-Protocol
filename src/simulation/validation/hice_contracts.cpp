#include "simulation/validation/hice_contracts.h"
#include <cmath>

namespace ailee {
namespace simulation {
namespace validation {

bool HiceValidator::check_practical_margin_ci(double ci_lower_bound, double practical_margin) {
    return ci_lower_bound > practical_margin;
}

HiceResult HiceValidator::evaluate_hice_contracts(
    const HiceMetrics& metrics,
    const HiceThresholds& thresholds,
    double practical_margin
) {
    HiceResult result;

    // HICE2: Covariance error < 10^-6
    result.hice2_ok = std::abs(metrics.covariance_error) < thresholds.max_covariance_error;

    // HICE3: Spectral drift < 10^-3 within eps_adj
    result.hice3_ok = std::abs(metrics.spectral_drift) < thresholds.max_spectral_drift;

    // HICE4: Delta memory < -0.05
    // Interpreted as the change in memory should not drop below -0.05
    result.hice4_ok = metrics.delta_memory >= thresholds.min_delta_memory;

    // HICE5: Context leakage < 0.01
    result.hice5_ok = metrics.context_leakage < thresholds.max_context_leakage;

    // HICE6: Nulls matched within 5% tolerance
    result.hice6_ok = metrics.null_matching_rate >= thresholds.min_null_matching_rate;

    // Practical Margin CI Rule Lock-In (must apply across relevant simulation metrics, mainly delta_auc here)
    result.practical_margin_ok = check_practical_margin_ci(metrics.ci_lower_bound, practical_margin);

    // HICE7: Delta AUC >= 0.03 AND CI Rule passes
    result.hice7_ok = (metrics.delta_auc >= thresholds.min_delta_auc) && result.practical_margin_ok;

    return result;
}

} // namespace validation
} // namespace simulation
} // namespace ailee
