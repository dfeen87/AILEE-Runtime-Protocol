#include "ailee/compartments/CompartmentStateMachine.hpp"
#include <chrono>

namespace ailee::compartments {

bool CompartmentStateMachine::is_valid_transition(CompartmentState current, CompartmentState target) {
    if (current == target) {
        return true; // No-op transition
    }

    switch (current) {
        case CompartmentState::ISOLATED:
            return target == CompartmentState::MONITORED || target == CompartmentState::QUARANTINED;
        case CompartmentState::MONITORED:
            return target == CompartmentState::ACTIVE || target == CompartmentState::ISOLATED ||
                   target == CompartmentState::SUSPENDED || target == CompartmentState::QUARANTINED;
        case CompartmentState::ACTIVE:
            return target == CompartmentState::MONITORED || target == CompartmentState::SUSPENDED ||
                   target == CompartmentState::QUARANTINED;
        case CompartmentState::SUSPENDED:
            return target == CompartmentState::MONITORED || target == CompartmentState::ISOLATED ||
                   target == CompartmentState::QUARANTINED;
        case CompartmentState::QUARANTINED:
            return target == CompartmentState::ISOLATED; // Recovery requires returning to ISOLATED first
    }
    return false;
}

CompartmentStateMachine::CompartmentStateMachine() {
    // Default system compartments
    register_compartment("core-execution", "Core Execution Engine");
    register_compartment("governance-gate", "Governance Approval Gate");
    register_compartment("alcoa-ledger", "ALCOA Ledger Subsystem");
    register_compartment("network-relay", "P2P Network & Relay");

    // Set initial default active state for system compartments with force=true
    transition_state("core-execution", CompartmentState::ACTIVE, "System initialization", true);
    transition_state("governance-gate", CompartmentState::ACTIVE, "System initialization", true);
    transition_state("alcoa-ledger", CompartmentState::ACTIVE, "System initialization", true);
    transition_state("network-relay", CompartmentState::MONITORED, "System initialization", true);
}

bool CompartmentStateMachine::register_compartment(const std::string& id, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (compartments_.find(id) != compartments_.end()) {
        return false;
    }

    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    CompartmentInfo info;
    info.compartment_id = id;
    info.name = name;
    info.state = CompartmentState::ISOLATED;
    info.last_transition_timestamp = now;
    info.isolation_policy = "default-strict-isolation";

    compartments_[id] = info;
    return true;
}

bool CompartmentStateMachine::transition_state(const std::string& id, CompartmentState new_state, const std::string& reason, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = compartments_.find(id);
    if (it == compartments_.end()) {
        return false;
    }

    if (!force && !is_valid_transition(it->second.state, new_state)) {
        return false; // Invalid transition rejected
    }

    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    it->second.state = new_state;
    it->second.last_transition_timestamp = now;
    if (!reason.empty()) {
        it->second.isolation_policy = reason;
    }
    return true;
}

CompartmentInfo CompartmentStateMachine::get_compartment(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = compartments_.find(id);
    if (it != compartments_.end()) {
        return it->second;
    }
    return CompartmentInfo{};
}

std::vector<CompartmentInfo> CompartmentStateMachine::list_compartments() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CompartmentInfo> result;
    for (const auto& [id, info] : compartments_) {
        result.push_back(info);
    }
    return result;
}

bool CompartmentStateMachine::is_isolated(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = compartments_.find(id);
    if (it != compartments_.end()) {
        return it->second.state == CompartmentState::ISOLATED ||
               it->second.state == CompartmentState::QUARANTINED;
    }
    return true; // Unknown compartments default to isolated for security
}

std::string CompartmentStateMachine::state_to_string(CompartmentState state) {
    switch (state) {
        case CompartmentState::ISOLATED: return "ISOLATED";
        case CompartmentState::MONITORED: return "MONITORED";
        case CompartmentState::ACTIVE: return "ACTIVE";
        case CompartmentState::SUSPENDED: return "SUSPENDED";
        case CompartmentState::QUARANTINED: return "QUARANTINED";
    }
    return "UNKNOWN";
}

} // namespace ailee::compartments
