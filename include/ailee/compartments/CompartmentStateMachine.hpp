#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

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
    bool transition_state(const std::string& id, CompartmentState new_state, const std::string& reason = "");
    CompartmentInfo get_compartment(const std::string& id) const;
    std::vector<CompartmentInfo> list_compartments() const;
    bool is_isolated(const std::string& id) const;

    static std::string state_to_string(CompartmentState state);

private:
    std::map<std::string, CompartmentInfo> compartments_;
};

} // namespace ailee::compartments
