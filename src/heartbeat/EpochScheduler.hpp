#ifndef AILEE_HEARTBEAT_EPOCH_SCHEDULER_HPP
#define AILEE_HEARTBEAT_EPOCH_SCHEDULER_HPP

#include <cstdint>

namespace ailee {
namespace heartbeat {

class EpochScheduler {
public:
    EpochScheduler();

    uint64_t get_current_epoch() const;
    void advance_epoch();

private:
    uint64_t current_epoch_;
};

} // namespace heartbeat
} // namespace ailee

#endif // AILEE_HEARTBEAT_EPOCH_SCHEDULER_HPP
