#pragma once

#include <cstdint>

namespace ailee {

struct ConfigInfo {
    uint8_t config_hash[32];
    uint32_t protocol_version;
};

} // namespace ailee
