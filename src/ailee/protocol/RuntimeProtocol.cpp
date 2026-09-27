#include "ailee/protocol/RuntimeProtocol.hpp"

namespace ailee::protocol {

RuntimeProtocol::RuntimeProtocol()
    : approval_gate_(constitution_) {}

RuntimeState RuntimeProtocol::evaluate_and_record(const posture::PostureEvaluationInput& posture_input,
                                                  const approval::ApprovalFactors& approval_factors,
                                                  const std::string& operator_id,
                                                  const std::string& operator_sig) {
    RuntimeState state;
    state.version = "37.1.0";

    // 1. Posture Evaluation
    state.posture = posture_engine_.evaluate(posture_input);

    // 2. Interpretation
    interpretation::InterpretationInput interp_input;
    interp_input.volatility = posture_input.recent_volatility;
    interp_input.fee_rate = posture_input.current_fee_rate;
    interp_input.high_fee_band = posture_input.high_fee_band;
    interp_input.block_lag_seconds = posture_input.time_since_last_block;
    interp_input.coherence_score = posture_input.signal_coherence;

    state.interpretation = interpretation_engine_.interpret(interp_input);

    // 3. Approval Gate Evaluation
    state.last_gate_decision = approval_gate_.evaluate_factors(approval_factors);

    // 4. Record in ALCOA Ledger
    ledger::AlcoaEntry entry;
    entry.operator_id = operator_id;
    entry.system_id = "ailee-v37-core";
    entry.operator_signature = operator_sig;
    entry.human_readable_summary = state.posture.summary;
    entry.regime_label = state.posture.regime_id;
    entry.compartment_label = "core-execution";
    entry.epoch_id = 3700;
    entry.epoch_hash = "0x" + state.posture.regime_id + "-hash-v37";
    entry.source_system = "AILEE-Trust-Layer-37.1.0";
    entry.posture_regime_id = state.posture.regime_id;
    entry.posture_score = state.posture.risk_score;
    entry.zk_recursion_root = approval_factors.zk_recursion_root.empty() ? "0x0000000000000000000000000000000000000000" : approval_factors.zk_recursion_root;
    entry.temporal_coherence_index = state.posture.temporal_coherence_index;
    entry.signal_energy = posture_input.current_fee_rate;
    entry.coherence_score = posture_input.signal_coherence;

    ledger_.record_entry(entry);
    state.last_ledger_entry = ledger_.get_latest_entry();

    state.healthy = state.last_gate_decision.approved && (state.interpretation.regime == interpretation::InterpretationRegime::NOMINAL);

    return state;
}

} // namespace ailee::protocol
