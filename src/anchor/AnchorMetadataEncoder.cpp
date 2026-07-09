#include "AnchorMetadataEncoder.h"
#include <openssl/sha.h>
#include <vector>

namespace ailee {
namespace anchor {

std::array<uint8_t, 32> AnchorMetadataEncoder::hash_metadata(const AnchorMetadata& meta) {
    std::vector<uint8_t> bytes = meta.to_bytes();
    std::array<uint8_t, 32> hash;

    // Use OpenSSL SHA256 directly for deterministic raw binary hashing
    SHA256(bytes.data(), bytes.size(), hash.data());

    return hash;
}

} // namespace anchor
} // namespace ailee
