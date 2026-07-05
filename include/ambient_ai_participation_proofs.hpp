#ifndef AMBIENT_AI_PARTICIPATION_PROOFS_HPP
#define AMBIENT_AI_PARTICIPATION_PROOFS_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include "ambient_ai_participation_measurement.hpp"

namespace ailee {
namespace participation {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientParticipationProofHeader {
    std::string publicKeyHex;         // Binds directly to IdentityPayload::publicKeyHex
    uint64_t epochId;                 // Monotonically increasing L2 epoch counter
    Hash256 summaryHash;              // Hash of the AmbientContributionSummary
    int64_t derivedEnergyScore;       // Sourced from energy::EnergyProfile::calculateEpochEnergy()

    // Deterministic binary serialization
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

struct AmbientParticipationProof {
    AmbientParticipationProofHeader header;
    AmbientContributionSummary summary;
    std::vector<uint8_t> ecdsaSignature; // Strict DER-encoded secp256k1 signature over header.hash()

    // Validation rules:
    // 1. Verifies the ECDSA signature matches publicKeyHex.
    // 2. Asserts summary.hash() == header.summaryHash.
    // 3. Asserts derivedEnergyScore >= 0.
    // 4. Asserts limits via ParticipationLimits.
    bool verify() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_PROOFS_HPP
