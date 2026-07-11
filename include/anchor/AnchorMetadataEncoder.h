#pragma once

#include "AnchorMetadata.h"
#include <array>
#include <cstdint>

namespace ailee {
namespace anchor {

class AnchorMetadataEncoder {
public:
    static std::array<uint8_t, 32> hash_metadata(const AnchorMetadata& meta);
};

} // namespace anchor
} // namespace ailee
