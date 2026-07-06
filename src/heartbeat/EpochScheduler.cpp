#include "EpochScheduler.hpp"

namespace ailee {
namespace heartbeat {

EpochScheduler::EpochScheduler() : current_epoch_(0) {}

uint64_t EpochScheduler::get_current_epoch() const {
    return current_epoch_;
}

void EpochScheduler::advance_epoch() {
    current_epoch_++;
}

} // namespace heartbeat
} // namespace ailee
