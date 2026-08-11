// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
#pragma once

#include <atomic>
#include <cstdint>

namespace ailee::network {

/**
 * Deterministic Logical Clock
 * Used to replace std::chrono for timestamping and rate-limiting
 * to ensure determinism across nodes without wall-clock dependency.
 */
class LogicalClock {
public:
    static uint64_t next() {
        return clock_.fetch_add(1, std::memory_order_relaxed);
    }

    static uint64_t now() {
        return clock_.load(std::memory_order_relaxed);
    }

private:
    static std::atomic<uint64_t> clock_;
};

} // namespace ailee::network
