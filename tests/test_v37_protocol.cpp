#include "ailee/protocol/RuntimeProtocol.hpp"
#include <gtest/gtest.h>

TEST(V37ProtocolTest, FullPipelineEvaluation) {
    ailee::protocol::RuntimeProtocol protocol;

    ailee::posture::PostureEvaluationInput posture_input;
    posture_input.current_fee_rate = 25.0;
    posture_input.high_fee_band = 50.0;
    posture_input.recent_volatility = 0.2;
    posture_input.block_interval_avg = 600;
    posture_input.time_since_last_block = 300;
    posture_input.daily_change_pct = 1.0;
    posture_input.signal_coherence = 0.95;

    ailee::approval::ApprovalFactors factors;
    factors.quorum_count = 4;
    factors.total_validators = 5;
    factors.operator_signature_valid = true;
    factors.system_signature_valid = true;
    factors.zk_state_consistent = true;
    factors.posture_score = 1.5;
    factors.temporal_coherence_index = 0.95;
    factors.zk_recursion_root = "0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0";

    auto state = protocol.evaluate_and_record(posture_input, factors, "operator-alice", "sig-valid-123");

    EXPECT_EQ(state.version, "37.0.0");
    EXPECT_TRUE(state.healthy);
    EXPECT_TRUE(state.last_gate_decision.approved);
    EXPECT_EQ(state.last_gate_decision.evaluated_factors.size(), 5u);

    // Ledger verification
    EXPECT_EQ(protocol.ledger().size(), 1u);
    auto entry = protocol.ledger().get_latest_entry();
    EXPECT_EQ(entry.operator_id, "operator-alice");
    EXPECT_EQ(entry.system_id, "ailee-v37-core");
    EXPECT_TRUE(protocol.ledger().verify_entry(entry.entry_id));

    // Compartment state machine verification
    EXPECT_TRUE(protocol.compartment_state_machine().is_isolated("unknown-comp"));
    EXPECT_FALSE(protocol.compartment_state_machine().is_isolated("core-execution"));
}

TEST(V37ProtocolTest, ApprovalGateRejectionOnLowQuorum) {
    ailee::approval::ApprovalGate gate;
    ailee::approval::ApprovalFactors factors;
    factors.quorum_count = 1; // Below threshold of 3
    factors.operator_signature_valid = true;
    factors.system_signature_valid = true;
    factors.zk_state_consistent = true;
    factors.posture_score = 1.0;
    factors.temporal_coherence_index = 0.9;

    auto decision = gate.evaluate_factors(factors);
    EXPECT_FALSE(decision.approved);
    EXPECT_FALSE(decision.quorum_passed);
    EXPECT_EQ(decision.rejection_reason, "Quorum threshold not met");
}

TEST(V37InterpretationTest, RegimeClassification) {
    ailee::interpretation::InterpretationEngine engine;
    ailee::interpretation::InterpretationInput input;
    input.block_lag_seconds = 4000; // Critical lag

    auto result = engine.interpret(input);
    EXPECT_EQ(result.regime, ailee::interpretation::InterpretationRegime::CRITICAL_ISOLATION);
    EXPECT_EQ(result.regime_name, "CRITICAL_ISOLATION");
}
