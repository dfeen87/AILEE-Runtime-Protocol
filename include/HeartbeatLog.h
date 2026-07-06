#pragma once

#include <cstdint>

namespace ailee {

class HeartbeatLog {
public:
    virtual ~HeartbeatLog() = default;
    virtual uint64_t get_latest_epoch() const = 0;
};

} // namespace ailee
