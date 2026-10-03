#pragma once

#include "ailee/governance/GovernanceConstitution.hpp"
#include "ailee/posture/PostureEngine.hpp"
#include "ailee/interpretation/InterpretationEngine.hpp"
#include "ailee/approval/ApprovalGate.hpp"
#include "ailee/ledger/AlcoaLedger.hpp"
#include "ailee/compartments/CompartmentStateMachine.hpp"

#include <memory>
#include <string>

namespace ailee::protocol {

struct RuntimeState {
    std::string version{"38.0.0"};
    posture::PostureResult posture;
    interpretation::InterpretationResult interpretation;
    approval::GateDecision last_gate_decision;
    ledger::AlcoaEntry last_ledger_entry;
    bool healthy{true};
};

class RuntimeProtocol {
public:
    RuntimeProtocol();

    RuntimeState evaluate_and_record(const posture::PostureEvaluationInput& posture_input,
                                     const approval::ApprovalFactors& approval_factors,
                                     const std::string& operator_id,
                                     const std::string& operator_sig);

    posture::PostureEngine& posture_engine() { return posture_engine_; }
    interpretation::InterpretationEngine& interpretation_engine() { return interpretation_engine_; }
    approval::ApprovalGate& approval_gate() { return approval_gate_; }
    ledger::AlcoaLedger& ledger() { return ledger_; }
    compartments::CompartmentStateMachine& compartment_state_machine() { return compartment_sm_; }

private:
    governance::GovernanceConstitution constitution_;
    posture::PostureEngine posture_engine_;
    interpretation::InterpretationEngine interpretation_engine_;
    approval::ApprovalGate approval_gate_;
    ledger::AlcoaLedger ledger_;
    compartments::CompartmentStateMachine compartment_sm_;
};

} // namespace ailee::protocol
