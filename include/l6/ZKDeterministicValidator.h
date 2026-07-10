#pragma once

#include <string>
#include "l6/ZKMetadata.h"

namespace ailee::l6 {

enum class DeterministicZKStatus : uint8_t {
    OK = 0,
    CONSTRAINT_MISMATCH,
    TRANSCRIPT_MISMATCH,
    EMPTY_CONSTRAINTS,
    EMPTY_TRANSCRIPT
};

struct DeterministicZKCheck {
    DeterministicZKStatus status;
    std::string proof_id;
    std::string constraint_set_id;
    std::string transcript_id;
};

DeterministicZKCheck validate_zk_metadata(
    const ZKProofMetadata& proof,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
);

} // namespace ailee::l6
