#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ailee::l6 {

struct ZKConstraintSet {
    std::string id;              // deterministic identifier for the constraint set
    uint64_t num_constraints;    // number of constraints (logical, not enforced yet)

    std::vector<uint8_t> to_bytes() const;
};

struct ZKTranscript {
    std::string id;              // deterministic identifier for the transcript
    uint64_t num_rounds;         // number of interaction rounds (logical)

    std::vector<uint8_t> to_bytes() const;
};

struct ZKProofMetadata {
    std::string proof_id;            // deterministic identifier
    std::string constraint_set_id;   // must match constraint set
    std::string transcript_id;       // must match transcript
    uint64_t logical_size_bytes;     // deterministic logical size

    std::vector<uint8_t> to_bytes() const;
};

} // namespace ailee::l6
