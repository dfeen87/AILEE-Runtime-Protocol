#pragma once

#include <vector>
#include <array>
#include <string>
#include <cstdint>

namespace ailee {
namespace anchor {

struct AnchorCommitValidationResult {
    bool ok;
    std::string error;
};

class AnchorCommitValidator {
public:
    static AnchorCommitValidationResult validate_anchor_commit(
        const std::vector<uint8_t>& signed_tx,
        const std::array<uint8_t, 32>& anchor_root,
        const std::array<uint8_t, 32>& metadata_hash,
        const std::array<uint8_t, 32>& internal_key,
        uint64_t value_sats
    );
};

} // namespace anchor
} // namespace ailee
