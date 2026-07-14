#include <gtest/gtest.h>
#include "l6/IslaRuntimeOrchestrator.h"
#include "l6/RuntimeEnvironment.h"
#include "l6/ZKMockBackend.h"
#include "l1_sync/mainnet_sync.hpp"
#include "simulation/validation/hice_contracts.h"
#include <vector>
#include <memory>

using namespace ailee::l6;
using namespace ailee::l1_sync;
using namespace ailee::simulation::validation;

class DeterministicSchedulerStub : public EpochScheduler {
public:
    SchedulerDecision get_decision(uint64_t epoch_id) const override {
        return {AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF};
    }
};

TEST(V27SimulationStress, MultiNodeDeterministicReplay) {
    RuntimeEnvironment env;
    env.is_ci = true;

    const int NODE_COUNT = 3;
    const int EPOCH_COUNT = 100;

    std::vector<std::unique_ptr<IslaRuntimeOrchestrator>> nodes;
    for (int i = 0; i < NODE_COUNT; ++i) {
        auto node = std::make_unique<IslaRuntimeOrchestrator>(env);
        ZKBackendConfig mock_config{ZKBackendType::MOCK, "stress_circuit", "", "", ""};
        node->attach_backend(mock_config);
        node->attach_scheduler(std::make_unique<DeterministicSchedulerStub>());
        nodes.push_back(std::move(node));
    }

    ZKConstraintSet constraints{"mock_constraints", 100};
    ZKTranscript transcript{"mock_transcript", 10};

    std::vector<IslaEpochResult> final_results(NODE_COUNT);

    uint64_t base_timestamp = 1600000000;

    for (int e = 0; e < EPOCH_COUNT; ++e) {
        EpochIntegrationBundle bundle{};
        bundle.sequence_id = e + 1;
        bundle.epoch_id = 1000 + e;
        bundle.state_root_hash = "deadbeef1234567890abcdef1234567890abcdef1234567890abcdef12345678";

        // Exact 600s intervals to avoid heartbeat drift
        bundle.clock_snapshot = {static_cast<uint64_t>(5000 + e), base_timestamp + (e * 600), "0000000000000000000000000000000000000000000000000000000000000000", "mock"};

        // Structurally valid mock
        bundle.anchor_input.internal_key.fill(1);
        bundle.anchor_input.prev_txid.fill(2);
        bundle.anchor_input.prev_vout = 0;
        bundle.anchor_input.value_sats = 10000;
        bundle.fee_sats = 500;
        bundle.constraints = &constraints;
        bundle.transcript = &transcript;
        bundle.hice_metrics = {
            0.5e-6, // covariance_error
            0.5e-3, // spectral_drift
            -0.01,  // delta_memory
            0.005,  // context_leakage
            0.98,   // null_matching_rate
            0.05,   // delta_auc
            0.04,   // ci_lower_bound
            0.05    // ci_point_estimate
        };

        for (int i = 0; i < NODE_COUNT; ++i) {
            final_results[i] = nodes[i]->run_epoch(bundle);
        }
    }

    // Compare all nodes
    for (int i = 1; i < NODE_COUNT; ++i) {
        EXPECT_EQ(final_results[0].metadata_hash, final_results[i].metadata_hash);
        EXPECT_EQ(final_results[0].anchor_tx.raw_tx, final_results[i].anchor_tx.raw_tx);
        EXPECT_EQ(final_results[0].validation.ok, final_results[i].validation.ok);
    }
}

TEST(V27SimulationStress, HighFrequencyPassiveSyncAndHICE) {
    MainnetSyncManager sync_manager;

    HeaderBatch headers;
    MempoolDeltaBatch deltas;
    for (int i = 0; i < 500; ++i) {
        BlockHeader h;
        h.version = 1;
        h.height = 10000 + i;
        h.timestamp = 1600000000 + (i * 60); // Simulated 1 min blocks for high frequency
        h.prev_hash.fill(i % 255);
        h.hash.fill((i + 1) % 255);
        headers.push_back(h);

        MempoolDelta d;
        d.is_add = true;
        d.txid.fill(i % 255);
        d.fee = 100 + i;
        d.size = 250;
        deltas.push_back(d);
    }

    sync_manager.ingest_headers(headers);
    sync_manager.ingest_mempool_deltas(deltas);

    std::string utxo_hash = sync_manager.compute_utxo_reflection_hash();
    EXPECT_FALSE(utxo_hash.empty());

    HiceMetrics metrics;
    metrics.covariance_error = 0.5e-6; // Mock optimal
    metrics.spectral_drift = sync_manager.get_spectral_drift(); // From system
    metrics.delta_memory = -0.01;
    metrics.context_leakage = 0.001;
    metrics.null_matching_rate = 0.99;
    metrics.delta_auc = 0.05; // Force pass
    metrics.ci_lower_bound = 0.03;
    metrics.ci_point_estimate = 0.04;

    double practical_margin = 0.02;
    HiceThresholds thresholds = HiceThresholds::v27_defaults();

    HiceResult result = HiceValidator::evaluate_hice_contracts(metrics, thresholds, practical_margin);

    EXPECT_TRUE(result.all_ok());
}
