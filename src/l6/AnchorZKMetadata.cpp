#include "l6/AnchorZKMetadata.h"

namespace ailee::l6 {

namespace {

void write_uint64_le(std::vector<uint8_t>& out, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((val >> (8 * i)) & 0xFF));
    }
}

void write_string(std::vector<uint8_t>& out, const std::string& str) {
    out.insert(out.end(), str.begin(), str.end());
}

} // namespace

std::vector<uint8_t> ZKAnchorMetadata::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, proof_id);
    write_string(result, constraint_set_id);
    write_string(result, transcript_id);
    result.push_back(static_cast<uint8_t>(validation_status));
    return result;
}

std::vector<uint8_t> AnchorPayload::to_bytes() const {
    std::vector<uint8_t> result;
    write_uint64_le(result, epoch_id);
    write_string(result, state_root_hash);

    std::vector<uint8_t> zk_bytes = zk_metadata.to_bytes();
    result.insert(result.end(), zk_bytes.begin(), zk_bytes.end());

    return result;
}

AnchorPayload attach_zk_to_anchor(
    const AnchorPayload& base_payload,
    const ProofPlan& proof_plan,
    const ZKProofMetadata* proof_metadata,
    const ZKConstraintSet* constraints,
    const ZKTranscript* transcript
) {
    AnchorPayload new_payload;
    new_payload.epoch_id = base_payload.epoch_id;
    new_payload.state_root_hash = base_payload.state_root_hash;

    if (proof_plan.decision == ProofDecision::ATTACH_PROOF && proof_metadata != nullptr) {
        if (constraints != nullptr && transcript != nullptr) {
            auto check = validate_zk_metadata(*proof_metadata, *constraints, *transcript);
            if (check.status == DeterministicZKStatus::OK) {
                new_payload.zk_metadata.constraint_set_id = proof_metadata->constraint_set_id;
                new_payload.zk_metadata.transcript_id = proof_metadata->transcript_id;
                new_payload.zk_metadata.proof_id = proof_metadata->proof_id;
                new_payload.zk_metadata.validation_status = DeterministicZKStatus::OK;
            } else {
                new_payload.zk_metadata.constraint_set_id = "";
                new_payload.zk_metadata.transcript_id = "";
                new_payload.zk_metadata.proof_id = "";
                new_payload.zk_metadata.validation_status = check.status;
            }
        } else {
            new_payload.zk_metadata.constraint_set_id = "";
            new_payload.zk_metadata.transcript_id = "";
            new_payload.zk_metadata.proof_id = "";
            new_payload.zk_metadata.validation_status = (constraints == nullptr) ? DeterministicZKStatus::EMPTY_CONSTRAINTS : DeterministicZKStatus::EMPTY_TRANSCRIPT;
        }
    } else {
        new_payload.zk_metadata.constraint_set_id = "";
        new_payload.zk_metadata.transcript_id = "";
        new_payload.zk_metadata.proof_id = "";
        new_payload.zk_metadata.validation_status = DeterministicZKStatus::OK;
    }

    return new_payload;
}

} // namespace ailee::l6
