#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"
#include "l6/IslaRuntimeOrchestrator.h"
#include "l6/RuntimeEnvironment.h"
#include "l6/ZKMockBackend.h"
#include "l6/RuntimeEnvironment.h"
#include "l6/ZKBackendFactory.h"
#include "l6/Exceptions.h"

using namespace ailee::l6;

namespace {
    OrchestrationResult run_orchestrator(const OrchestrationContext& ctx, IZKProvingBackend* b, const ZKBackendConfig& cfg, const ZKConstraintSet* c, const ZKTranscript* t, const std::string& state_root) {
        RuntimeEnvironment env;
        env.is_ci = true;
        IslaRuntimeOrchestrator isla(env);

        EpochIntegrationBundle bundle{};
        bundle.anchor_input.internal_key.fill(0);
        bundle.anchor_input.prev_txid.fill(0);
        bundle.anchor_input.prev_vout = 0;
        bundle.anchor_input.value_sats = 0;

        bundle.epoch_id = ctx.epoch_id;
        bundle.state_root_hash = state_root;
        bundle.anchor_plan = ctx.anchor_plan;
        bundle.proof_plan = ctx.proof_plan;
        bundle.constraints = c;
        bundle.transcript = t;
        bundle.hice_metrics.covariance_error = 0.5e-6; bundle.hice_metrics.spectral_drift = 0.0; bundle.hice_metrics.delta_memory = -0.01; bundle.hice_metrics.context_leakage = 0.001; bundle.hice_metrics.null_matching_rate = 0.99; bundle.hice_metrics.delta_auc = 0.05; bundle.hice_metrics.ci_lower_bound = 0.04; bundle.hice_metrics.ci_point_estimate = 0.05;
        bundle.fee_sats = 1000;

        if (b) {
            isla.attach_backend(b, cfg);
        } else {
            isla.attach_backend(nullptr, cfg);
        }

        return isla.run_epoch(bundle).zk_result;
    }
}


TEST(ZKProvingIntegrationTest, OrchestrationAttachesValidProof) {
    ZKBackendConfig config{ZKBackendType::MOCK, "test_circuit", "", "", ""};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    std::string state_root_hash = "deadbeef";

    RuntimeEnvironment env;
    env.is_ci = true;
    config.type = select_backend_type(env, config);
    if (env.is_ci && config.type != ZKBackendType::MOCK) {
        throw DeterministicBackendException("Non-deterministic backend not allowed in CI");
    }
    auto active_backend = make_backend(config);

    auto result = run_orchestrator(ctx, active_backend.get(), config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.epoch_id, 42);
    EXPECT_EQ(payload.state_root_hash, "deadbeef");
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "constraint_1");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "transcript_1");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);

    // Proof commitment hash should not be empty
    EXPECT_FALSE(payload.proof_commitment_hash.empty());
}

TEST(ZKProvingIntegrationTest, OrchestrationLeavesEmptyWhenSkipped) {
    ZKBackendConfig config{ZKBackendType::MOCK, "test_circuit", "", "", ""};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 43;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::SKIP;

    std::string state_root_hash = "deadbeef";

    RuntimeEnvironment env;
    env.is_ci = true;
    config.type = select_backend_type(env, config);
    if (env.is_ci && config.type != ZKBackendType::MOCK) {
        throw DeterministicBackendException("Non-deterministic backend not allowed in CI");
    }
    auto active_backend = make_backend(config);

    auto result = run_orchestrator(ctx, active_backend.get(), config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}
