#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>

namespace ailee::compartments {

enum class CompartmentState {
    ISOLATED,
    MONITORED,
    ACTIVE,
    SUSPENDED,
    QUARANTINED
};

struct CompartmentInfo {
    std::string compartment_id;
    std::string name;
    CompartmentState state{CompartmentState::ISOLATED};
    uint64_t last_transition_timestamp{0};
    std::string isolation_policy;
};

class CompartmentStateMachine {
public:
    CompartmentStateMachine();

    bool register_compartment(const std::string& id, const std::string& name);
    bool transition_state(const std::string& id, CompartmentState new_state, const std::string& reason = "", bool force = false);
    CompartmentInfo get_compartment(const std::string& id) const;
    std::vector<CompartmentInfo> list_compartments() const;
    bool is_isolated(const std::string& id) const;

    static bool is_valid_transition(CompartmentState current, CompartmentState target);
    static std::string state_to_string(CompartmentState state);

private:
    std::map<std::string, CompartmentInfo> compartments_;
    mutable std::mutex mutex_;
};

} // namespace ailee::compartments
