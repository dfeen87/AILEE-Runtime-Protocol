#pragma once

#include <cstdint>

namespace ailee {

class StateRootLog {
public:
    virtual ~StateRootLog() = default;
    virtual void get_latest_state_root(uint8_t out_root[32]) const = 0;
};

} // namespace ailee
