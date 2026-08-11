// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
#include "LogicalClock.h"

namespace ailee::network {

std::atomic<uint64_t> LogicalClock::clock_{0};

} // namespace ailee::network
