#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include "l6/ZKMetadata.h"
#include "l6/ZKDeterministicValidator.h"
#include "l6/ProofScheduling.h"

namespace ailee::l6 {

struct ZKAnchorMetadata {
    std::string proof_id;
    std::string constraint_set_id;
    std::string transcript_id;
    DeterministicZKStatus validation_status;

    std::vector<uint8_t> to_bytes() const;
};

struct AnchorPayload {
    uint64_t epoch_id;
    std::string state_root_hash;
    ZKAnchorMetadata zk_metadata;
    std::string proof_commitment_hash;

    std::vector<uint8_t> to_bytes() const;
};

} // namespace ailee::l6
