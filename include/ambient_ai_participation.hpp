#include <cstdint>
#ifndef AMBIENT_AI_PARTICIPATION_HPP
#define AMBIENT_AI_PARTICIPATION_HPP

#include <vector>
#include <string>

namespace ailee {
namespace identity {

struct BuildMetadata {
    std::string commitHash;
    uint32_t buildNumber;
    uint32_t protocolVersion;

    std::array<uint8_t, 32> hash() const;
};

struct ParticipationProof {
    std::string peerId;
    std::vector<uint8_t> nodeEcdsaSignature; // Sign( hash( epochId || BuildMetadata ) )
    BuildMetadata metadata;

    bool verify() const; // Validates signature and protocolVersion against consensus rules
};

} // namespace identity
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_HPP
