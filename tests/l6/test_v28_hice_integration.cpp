#include <gtest/gtest.h>
#include "l6/IslaRuntimeOrchestrator.h"
#include "l6/RuntimeEnvironment.h"
#include "l6/ZKMockBackend.h"

using namespace ailee::l6;

TEST(V28HiceIntegration, FailsOnBadHiceMetrics) {
    RuntimeEnvironment env;
    env.is_ci = true;
    IslaRuntimeOrchestrator isla(env);

    ZKBackendConfig mock_config{ZKBackendType::MOCK, "hice_circuit", "", "", ""};
    isla.attach_backend(mock_config);

    EpochIntegrationBundle bundle{};
    bundle.epoch_id = 1;
    bundle.state_root_hash = "deadbeef1234567890abcdef1234567890abcdef1234567890abcdef12345678";
    bundle.clock_snapshot = {1000, 1600000000, "0000000000000000000000000000000000000000000000000000000000000000", "mock"};

    // Explicitly failing metrics (e.g., covariance error too high)
    bundle.hice_metrics = {
        2e-6,   // covariance_error (> 1e-6 thresholds default) -> FAIL
        0.5e-3, // spectral_drift
        -0.01,  // delta_memory
        0.005,  // context_leakage
        0.98,   // null_matching_rate
        0.05,   // delta_auc
        0.04,   // ci_lower_bound
        0.05    // ci_point_estimate
    };

    bool caught = false;
    try {
        isla.run_epoch(bundle);
    } catch(const DeterministicBackendException& e) {
        caught = true;
        std::string expected = "HICE contract validation failed - see result flags and metrics";
        EXPECT_EQ(expected, std::string(e.what()));
    }
    EXPECT_TRUE(caught);
}
