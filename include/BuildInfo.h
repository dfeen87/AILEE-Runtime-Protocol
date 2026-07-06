#pragma once

#include <cstdint>

namespace ailee {
namespace build {

// We create a minimal POD struct for MeshView that avoids std::string.
struct BuildHashInfo {
    uint8_t build_hash[32];
    uint8_t protocol_version_hash[32];
};

} // namespace build

struct BuildInfo {
    uint8_t build_hash[32];
    uint32_t build_number;
};

} // namespace ailee
