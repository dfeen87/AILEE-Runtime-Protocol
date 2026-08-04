// Licensed under the PolyForm Noncommercial License 1.0.0
#include "LogicalClock.h"

namespace ailee::network {

std::atomic<uint64_t> LogicalClock::clock_{0};

} // namespace ailee::network
