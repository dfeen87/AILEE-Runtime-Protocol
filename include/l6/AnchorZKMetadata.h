#pragma once

#include <string>
#include <cstdint>
#include "l6/ZKStubInterface.h"
#include "l6/ProofScheduling.h"

namespace ailee::l6 {

struct ZKAnchorMetadata {
    std::string constraint_set_id;
    std::string transcript_id;
    std::string proof_id;
};

struct AnchorPayload {
    uint64_t epoch_id;
    std::string state_root_hash;
    ZKAnchorMetadata zk_metadata;
};

AnchorPayload attach_zk_to_anchor(
    const AnchorPayload& base_payload,
    const ProofPlan& proof_plan,
    const ZKProofStub* proof_stub
);

} // namespace ailee::l6
