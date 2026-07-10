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

    write_string(result, proof_commitment_hash);

    return result;
}

} // namespace ailee::l6
