#include "l6/AnchorSerializer.h"
#include <cstring>

namespace ailee {
namespace l6 {

namespace {

void encode_u64_be(uint64_t val, uint8_t* out) {
    out[0] = (val >> 56) & 0xFF;
    out[1] = (val >> 48) & 0xFF;
    out[2] = (val >> 40) & 0xFF;
    out[3] = (val >> 32) & 0xFF;
    out[4] = (val >> 24) & 0xFF;
    out[5] = (val >> 16) & 0xFF;
    out[6] = (val >> 8)  & 0xFF;
    out[7] = (val)       & 0xFF;
}

uint64_t decode_u64_be(const uint8_t* in) {
    return (static_cast<uint64_t>(in[0]) << 56) |
           (static_cast<uint64_t>(in[1]) << 48) |
           (static_cast<uint64_t>(in[2]) << 40) |
           (static_cast<uint64_t>(in[3]) << 32) |
           (static_cast<uint64_t>(in[4]) << 24) |
           (static_cast<uint64_t>(in[5]) << 16) |
           (static_cast<uint64_t>(in[6]) << 8)  |
           (static_cast<uint64_t>(in[7]));
}

} // namespace

std::vector<uint8_t> serialize(const AnchorRecord& rec) {
    std::vector<uint8_t> out(49);
    
    out[0] = rec.version;
    encode_u64_be(rec.replay_height, out.data() + 1);
    encode_u64_be(rec.coherence_score, out.data() + 9);
    std::memcpy(out.data() + 17, rec.state_root.data(), 32);
    
    return out;
}

bool deserialize(const std::vector<uint8_t>& bytes, AnchorRecord& out) {
    if (bytes.size() != 49) {
        return false;
    }
    
    out.version = bytes[0];
    out.replay_height = decode_u64_be(bytes.data() + 1);
    out.coherence_score = decode_u64_be(bytes.data() + 9);
    std::memcpy(out.state_root.data(), bytes.data() + 17, 32);
    
    return true;
}

} // namespace l6
} // namespace ailee
