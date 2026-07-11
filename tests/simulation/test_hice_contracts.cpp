#include <gtest/gtest.h>
#include "simulation/validation/hice_contracts.h"

using namespace ailee::simulation::validation;

TEST(HiceContractsTest, AllPassingMetrics) {
    HiceMetrics metrics = {
        0.5e-6, // covariance_error (HICE2 < 1e-6)
        0.5e-3, // spectral_drift (HICE3 < 1e-3)
        -0.02,  // delta_memory (HICE4 >= -0.05)
        0.005,  // context_leakage (HICE5 < 0.01)
        0.98,   // null_matching_rate (HICE6 >= 0.95)
        0.04,   // delta_auc (HICE7 >= 0.03)
        0.035,  // ci_lower_bound (Practical Margin CI > 0.02)
        0.04    // ci_point_estimate
    };

    HiceThresholds thresholds = HiceThresholds::v27_defaults();
    double practical_margin = 0.02;

    HiceResult result = HiceValidator::evaluate_hice_contracts(metrics, thresholds, practical_margin);

    EXPECT_TRUE(result.hice2_ok);
    EXPECT_TRUE(result.hice3_ok);
    EXPECT_TRUE(result.hice4_ok);
    EXPECT_TRUE(result.hice5_ok);
    EXPECT_TRUE(result.hice6_ok);
    EXPECT_TRUE(result.hice7_ok);
    EXPECT_TRUE(result.practical_margin_ok);
    EXPECT_TRUE(result.all_ok());
}

TEST(HiceContractsTest, FailsOnCovarianceError) {
    HiceMetrics metrics = { 2e-6, 0.5e-3, -0.02, 0.005, 0.98, 0.04, 0.035, 0.04 };
    HiceResult result = HiceValidator::evaluate_hice_contracts(metrics, HiceThresholds::v27_defaults(), 0.02);

    EXPECT_FALSE(result.hice2_ok);
    EXPECT_FALSE(result.all_ok());
}

TEST(HiceContractsTest, FailsOnPracticalMarginCI_LowerBoundRule) {
    // Here, point estimate (0.04) passes Practical Margin (0.02),
    // but lower bound (0.01) does not. This should fail to enforce conservative stance.
    HiceMetrics metrics = { 0.5e-6, 0.5e-3, -0.02, 0.005, 0.98, 0.04, 0.01, 0.04 };
    HiceResult result = HiceValidator::evaluate_hice_contracts(metrics, HiceThresholds::v27_defaults(), 0.02);

    EXPECT_FALSE(result.practical_margin_ok);
    EXPECT_FALSE(result.hice7_ok);
    EXPECT_FALSE(result.all_ok());
}

TEST(HiceContractsTest, FailsOnDeltaMemory) {
    HiceMetrics metrics = { 0.5e-6, 0.5e-3, -0.06, 0.005, 0.98, 0.04, 0.035, 0.04 };
    HiceResult result = HiceValidator::evaluate_hice_contracts(metrics, HiceThresholds::v27_defaults(), 0.02);

    EXPECT_FALSE(result.hice4_ok);
    EXPECT_FALSE(result.all_ok());
}

TEST(HiceContractsTest, FailsOnNullMatchingRate) {
    HiceMetrics metrics = { 0.5e-6, 0.5e-3, -0.02, 0.005, 0.94, 0.04, 0.035, 0.04 };
    HiceResult result = HiceValidator::evaluate_hice_contracts(metrics, HiceThresholds::v27_defaults(), 0.02);

    EXPECT_FALSE(result.hice6_ok);
    EXPECT_FALSE(result.all_ok());
}
