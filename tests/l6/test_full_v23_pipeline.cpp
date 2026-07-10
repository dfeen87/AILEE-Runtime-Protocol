#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"
#include "l6/ZKMockBackend.h"
#include "l6/DeterministicEpochAnchoring.h"
#include "l6/ProofScheduling.h"

using namespace ailee::l6;

class FullV23PipelineTest : public ::testing::Test {
protected:
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::MOCK, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraints_v23_test", 100};
    ZKTranscript transcript{"transcript_v23_test", 10};
    std::string state_root_hash = "deadbeef";

    OrchestrationContext get_context(uint64_t epoch_id) {
        OrchestrationContext ctx;
        ctx.epoch_id = epoch_id;

        // Use realistic dummy scheduling for integration
        // For testing, let's say we anchor every 2 epochs and prove every 4
        if (epoch_id % 2 == 0) {
            ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
            ctx.anchor_plan.anchor_cycle_id = epoch_id / 2;
        } else {
            ctx.anchor_plan.decision = AnchorDecision::SKIP;
            ctx.anchor_plan.anchor_cycle_id = epoch_id / 2; // Cycle ID monotonically increments based on interval
        }

        if (epoch_id % 4 == 0) {
            ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;
            ctx.proof_plan.proof_cycle_id = epoch_id / 4;
        } else {
            ctx.proof_plan.decision = ProofDecision::SKIP;
            ctx.proof_plan.proof_cycle_id = epoch_id / 4;
        }

        return ctx;
    }
};

TEST_F(FullV23PipelineTest, EndToEndPipelineDeterminismAndCrossPhaseConsistency) {
    // Phase 1: Run the pipeline
    auto ctx = get_context(4); // Epoch 4 -> Anchor + Prove
    auto result1 = orchestrate_epoch(ctx, &backend, config, &constraints, &transcript, state_root_hash);

    // Validate output of orchestration
    EXPECT_TRUE(result1.should_anchor);
    EXPECT_TRUE(result1.should_attach_proof);

    auto& p1 = result1.anchor_payload;
    EXPECT_EQ(p1.epoch_id, 4);
    EXPECT_EQ(p1.state_root_hash, state_root_hash);
    EXPECT_EQ(p1.zk_metadata.constraint_set_id, "constraints_v23_test");
    EXPECT_EQ(p1.zk_metadata.transcript_id, "transcript_v23_test");
    EXPECT_EQ(p1.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(p1.proof_commitment_hash.length(), 64);

    // Phase 2: Ensure deterministic replay
    auto result2 = orchestrate_epoch(ctx, &backend, config, &constraints, &transcript, state_root_hash);

    // Compare exact bytes to guarantee replay determinism
    EXPECT_EQ(result1.anchor_payload.to_bytes(), result2.anchor_payload.to_bytes());
}

TEST_F(FullV23PipelineTest, ProofSchedulingCycles) {
    // Test a full cycle of 5 epochs to verify scheduling effects on payload

    // Epoch 1: Skip Anchor, Skip Prove
    auto ctx1 = get_context(1);
    auto res1 = orchestrate_epoch(ctx1, &backend, config, &constraints, &transcript, state_root_hash);
    EXPECT_FALSE(res1.should_anchor);
    EXPECT_FALSE(res1.should_attach_proof);

    // Epoch 2: Anchor, Skip Prove
    auto ctx2 = get_context(2);
    auto res2 = orchestrate_epoch(ctx2, &backend, config, &constraints, &transcript, state_root_hash);
    EXPECT_TRUE(res2.should_anchor);
    EXPECT_FALSE(res2.should_attach_proof);
    EXPECT_EQ(res2.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(res2.anchor_payload.proof_commitment_hash, "");

    // Epoch 3: Skip Anchor, Skip Prove
    auto ctx3 = get_context(3);
    auto res3 = orchestrate_epoch(ctx3, &backend, config, &constraints, &transcript, state_root_hash);
    EXPECT_FALSE(res3.should_anchor);
    EXPECT_FALSE(res3.should_attach_proof);

    // Epoch 4: Anchor, Prove
    auto ctx4 = get_context(4);
    auto res4 = orchestrate_epoch(ctx4, &backend, config, &constraints, &transcript, state_root_hash);
    EXPECT_TRUE(res4.should_anchor);
    EXPECT_TRUE(res4.should_attach_proof);
    EXPECT_EQ(res4.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_NE(res4.anchor_payload.proof_commitment_hash, "");
}
