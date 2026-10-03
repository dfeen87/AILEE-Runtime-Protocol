#include "ailee/protocol/RuntimeProtocol.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <limits>

TEST(V37HardeningTest, PostureEngineDeterminismAndSanitization) {
    ailee::posture::PostureEngine engine;

    ailee::posture::PostureEvaluationInput input;
    input.current_fee_rate = 75.0;
    input.high_fee_band = 50.0;
    input.recent_volatility = 0.85;
    input.block_interval_avg = 950;
    input.time_since_last_block = 2000;
    input.daily_change_pct = 6.2;
    input.signal_coherence = 0.92;

    auto res1 = engine.evaluate(input);
    auto res2 = engine.evaluate(input);

    EXPECT_EQ(res1.risk_score, res2.risk_score);
    EXPECT_EQ(res1.regime, res2.regime);
    EXPECT_EQ(res1.regime_id, res2.regime_id);
    EXPECT_EQ(res1.confidence, res2.confidence);
    EXPECT_EQ(res1.temporal_coherence_index, res2.temporal_coherence_index);

    // Test sanitization with out-of-bounds/NaN inputs
    ailee::posture::PostureEvaluationInput dirty_input;
    dirty_input.current_fee_rate = -100.0;
    dirty_input.recent_volatility = 5.0; // Should clamp to 1.0
    dirty_input.signal_coherence = -0.5; // Should clamp to 0.0

    auto sanitized = engine.evaluate(dirty_input);
    EXPECT_GE(sanitized.risk_score, 0.0);
    EXPECT_LE(sanitized.risk_score, 10.0);
    EXPECT_GE(sanitized.temporal_coherence_index, 0.0);
    EXPECT_LE(sanitized.temporal_coherence_index, 1.0);
}

TEST(V37HardeningTest, ApprovalGateGovernanceHardening) {
    ailee::approval::ApprovalGate gate;

    ailee::approval::ApprovalFactors factors;
    factors.quorum_count = 4;
    factors.total_validators = 5;
    factors.operator_signature_valid = true;
    factors.system_signature_valid = true;
    factors.zk_state_consistent = true;
    factors.posture_score = 1.8;
    factors.temporal_coherence_index = 0.88;
    factors.zk_recursion_root = "0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0";

    // Valid factors pass
    auto decision = gate.evaluate_factors(factors);
    EXPECT_TRUE(decision.approved);

    // Test posture score threshold enforcement (e.g., posture_score > 2.5 fails)
    factors.posture_score = 3.5;
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);

    // Test zero-posture score is NOT a bypass vector when threshold is exceeded or strict
    factors.posture_score = 0.0;
    EXPECT_TRUE(gate.evaluate_factors(factors).approved); // 0.0 <= 2.5 -> PASS

    // Test quorum bounds failure (quorum_count > total_validators)
    factors.posture_score = 1.0;
    factors.quorum_count = 10;
    factors.total_validators = 5;
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);

    // Test stale or empty ZK recursion root failure
    factors.quorum_count = 4;
    factors.zk_recursion_root = "";
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);

    factors.zk_recursion_root = "0x0000000000000000000000000000000000000000000000000000000000000000";
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);
}

TEST(V37HardeningTest, ApprovalGateRejectsMalformedNumericEvidenceAndRoots) {
    ailee::approval::ApprovalGate gate;
    ailee::approval::ApprovalFactors factors;
    factors.quorum_count = 4;
    factors.total_validators = 5;
    factors.operator_signature_valid = true;
    factors.system_signature_valid = true;
    factors.zk_state_consistent = true;
    factors.posture_score = 1.0;
    factors.temporal_coherence_index = 0.9;
    factors.zk_recursion_root = "0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0";

    factors.posture_score = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);
    factors.posture_score = 1.0;

    factors.temporal_coherence_index = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);
    factors.temporal_coherence_index = 0.9;

    factors.zk_recursion_root = "not-a-cryptographic-root";
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);
}

TEST(V37HardeningTest, ApprovalGateRejectsInvalidConstitution) {
    ailee::governance::ConstitutionRules rules;
    rules.max_allowable_posture_score = std::numeric_limits<double>::infinity();
    ailee::approval::ApprovalGate gate{ailee::governance::GovernanceConstitution{rules}};
    ailee::approval::ApprovalFactors factors;
    EXPECT_FALSE(gate.evaluate_factors(factors).approved);
    EXPECT_EQ(gate.evaluate_factors(factors).rejection_reason, "Invalid governance constitution");
}

TEST(V37HardeningTest, AlcoaLedgerIntegrityAndChainVerification) {
    ailee::ledger::AlcoaLedger ledger;

    ailee::ledger::AlcoaEntry entry1;
    entry1.operator_id = "op-alice";
    entry1.system_id = "ailee-v37";
    entry1.operator_signature = "sig-1";
    entry1.human_readable_summary = "Entry 1";
    entry1.regime_label = "neutral";
    entry1.compartment_label = "core-execution";
    entry1.timestamp_utc = 1743000001;
    entry1.epoch_id = 1;
    entry1.epoch_hash = "0xhash1";
    entry1.posture_regime_id = "neutral";
    entry1.posture_score = 1.0;
    entry1.zk_recursion_root = "0xzkroot1";
    entry1.temporal_coherence_index = 0.95;

    std::string id1 = ledger.record_entry(entry1);
    EXPECT_FALSE(id1.empty());
    EXPECT_TRUE(ledger.verify_entry(id1));

    ailee::ledger::AlcoaEntry entry2;
    entry2.operator_id = "op-bob";
    entry2.system_id = "ailee-v37";
    entry2.operator_signature = "sig-2";
    entry2.human_readable_summary = "Entry 2";
    entry2.regime_label = "chop";
    entry2.compartment_label = "core-execution";
    entry2.timestamp_utc = 1743000002;
    entry2.epoch_id = 2;
    entry2.epoch_hash = "0xhash2";
    entry2.posture_regime_id = "chop";
    entry2.posture_score = 2.0;
    entry2.zk_recursion_root = "0xzkroot2";
    entry2.temporal_coherence_index = 0.90;

    std::string id2 = ledger.record_entry(entry2);
    EXPECT_FALSE(id2.empty());
    EXPECT_TRUE(ledger.verify_entry(id2));

    // Verify parent hash chain continuity
    EXPECT_TRUE(ledger.verify_chain());
}

TEST(V37HardeningTest, AlcoaLedgerDetectsContentAndIdentifierTampering) {
    ailee::ledger::AlcoaLedger ledger;
    ailee::ledger::AlcoaEntry entry;
    entry.operator_id = "op-alice";
    entry.system_id = "ailee-v37";
    entry.operator_signature = "sig-1";
    entry.human_readable_summary = "Original evidence";
    entry.regime_label = "neutral";
    entry.compartment_label = "core-execution";
    entry.timestamp_utc = 1743000001;
    entry.epoch_id = 1;
    entry.epoch_hash = "0xhash1";
    entry.posture_regime_id = "neutral";
    entry.posture_score = 1.0;
    entry.zk_recursion_root = "0xzkroot1";
    entry.temporal_coherence_index = 0.95;
    entry.entry_id = "caller-controlled-id";

    const auto id = ledger.record_entry(entry);
    EXPECT_NE(id, "caller-controlled-id");
    ASSERT_TRUE(ledger.verify_chain());

    auto& stored = const_cast<ailee::ledger::AlcoaEntry&>(ledger.entries().front());
    stored.human_readable_summary = "Tampered evidence";
    EXPECT_FALSE(ledger.verify_entry(id));
    EXPECT_FALSE(ledger.verify_chain());
}

TEST(V37HardeningTest, RuntimeBindsApprovalToAuthoritativePostureEvaluation) {
    ailee::protocol::RuntimeProtocol protocol;
    ailee::posture::PostureEvaluationInput telemetry;
    telemetry.current_fee_rate = 200.0;
    telemetry.high_fee_band = 50.0;
    telemetry.recent_volatility = 1.0;
    telemetry.block_interval_avg = 1200;
    telemetry.time_since_last_block = 4000;
    telemetry.signal_coherence = 0.1;

    ailee::approval::ApprovalFactors factors;
    factors.quorum_count = 4;
    factors.total_validators = 5;
    factors.operator_signature_valid = true;
    factors.system_signature_valid = true;
    factors.zk_state_consistent = true;
    factors.posture_score = 0.0; // Untrusted caller cannot override evaluated risk.
    factors.temporal_coherence_index = 1.0;
    factors.zk_recursion_root = "0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0";

    const auto state = protocol.evaluate_and_record(telemetry, factors, "operator", "signature");
    EXPECT_FALSE(state.last_gate_decision.approved);
    EXPECT_FALSE(state.healthy);
    EXPECT_FALSE(state.last_gate_decision.posture_passed);
    EXPECT_FALSE(state.last_gate_decision.coherence_passed);
}

TEST(V37HardeningTest, NonFiniteTelemetryFailsClosed) {
    ailee::posture::PostureEngine engine;
    ailee::posture::PostureEvaluationInput input;
    input.current_fee_rate = std::numeric_limits<double>::quiet_NaN();
    const auto result = engine.evaluate(input);
    EXPECT_EQ(result.risk_score, 10.0);
    EXPECT_EQ(result.temporal_coherence_index, 0.0);
}

TEST(V37HardeningTest, CompartmentStateMachineBoundaryTransitionRules) {
    ailee::compartments::CompartmentStateMachine sm;
    sm.register_compartment("isolated-comp", "Isolated Compartment"); // Starts in ISOLATED

    // ISOLATED -> ACTIVE (Invalid direct jump should fail)
    EXPECT_FALSE(sm.transition_state("isolated-comp", ailee::compartments::CompartmentState::ACTIVE));

    // ISOLATED -> MONITORED -> ACTIVE (Valid sequence)
    EXPECT_TRUE(sm.transition_state("isolated-comp", ailee::compartments::CompartmentState::MONITORED));
    EXPECT_TRUE(sm.transition_state("isolated-comp", ailee::compartments::CompartmentState::ACTIVE));

    // ACTIVE -> QUARANTINED -> ISOLATED (Recovery path)
    EXPECT_TRUE(sm.transition_state("isolated-comp", ailee::compartments::CompartmentState::QUARANTINED));
    EXPECT_FALSE(sm.transition_state("isolated-comp", ailee::compartments::CompartmentState::ACTIVE)); // Cannot go directly from QUARANTINED to ACTIVE
    EXPECT_TRUE(sm.transition_state("isolated-comp", ailee::compartments::CompartmentState::ISOLATED));
}

TEST(V37HardeningTest, CompartmentStateMachineConcurrencyStress) {
    ailee::compartments::CompartmentStateMachine sm;
    sm.register_compartment("test-comp", "Test Compartment");

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&sm, i]() {
            if (i % 2 == 0) {
                sm.transition_state("test-comp", ailee::compartments::CompartmentState::MONITORED, "Step 1");
            } else {
                sm.transition_state("test-comp", ailee::compartments::CompartmentState::QUARANTINED, "Step 2");
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto info = sm.get_compartment("test-comp");
    EXPECT_FALSE(info.compartment_id.empty());
}

TEST(V37HardeningTest, InterpretationEngineMathematicalStability) {
    ailee::interpretation::InterpretationEngine engine;

    ailee::interpretation::InterpretationInput input;
    input.volatility = 0.85;
    input.fee_rate = 120.0;
    input.high_fee_band = 50.0;
    input.block_lag_seconds = 4500; // Critical lag
    input.coherence_score = 0.20;

    auto res = engine.interpret(input);
    EXPECT_EQ(res.regime, ailee::interpretation::InterpretationRegime::CRITICAL_ISOLATION);
    EXPECT_FALSE(res.recommendation.empty());
    EXPECT_FALSE(res.triggered_rules.empty());
}
