#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace ailee {
namespace l6 {

enum class ReplayStatus {
    OK,
    MISMATCH,
    MISSING_ANCHOR
};

struct ReplayVerdict {
    ReplayStatus status;
    uint64_t epoch_id;
    std::string local_hash;
    std::string anchor_hash;
};

ReplayVerdict verify_epoch_against_anchor(
    uint64_t epoch_id,
    const std::string& local_epoch_hash,
    const std::optional<std::string>& anchor_hash_from_l1
);

} // namespace l6
} // namespace ailee
