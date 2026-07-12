#!/bin/bash
cat << 'INNER_EOF' > src/identity/ambient_ai_participation.cpp
#include <array>
#include <openssl/sha.h>
#include "ambient_ai_participation.hpp"
#include <string.h>

namespace ailee {
namespace identity {

std::array<uint8_t, 32> BuildMetadata::hash() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), commitHash.begin(), commitHash.end());
    auto push32 = [&out](uint32_t val) {
        for (int i = 3; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push32(buildNumber);
    push32(protocolVersion);

    std::array<uint8_t, 32> result;
    SHA256(out.data(), out.size(), result.data());
    return result;
}

// ParticipationProof::verify() is defined in ambient_ai_node_identity.cpp to avoid ODR violations

}
}
INNER_EOF
