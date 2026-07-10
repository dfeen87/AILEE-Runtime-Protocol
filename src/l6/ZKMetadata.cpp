#include "l6/ZKMetadata.h"

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

std::vector<uint8_t> ZKConstraintSet::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, id);
    write_uint64_le(result, num_constraints);
    return result;
}

std::vector<uint8_t> ZKTranscript::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, id);
    write_uint64_le(result, num_rounds);
    return result;
}

std::vector<uint8_t> ZKProofMetadata::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, proof_id);
    write_string(result, constraint_set_id);
    write_string(result, transcript_id);
    write_uint64_le(result, logical_size_bytes);
    return result;
}

} // namespace ailee::l6
