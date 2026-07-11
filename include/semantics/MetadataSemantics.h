#pragma once

#include <cstdint>
#include <string>
#include <array>
#include "l6/ZKProvingBackend.h"

namespace ailee::l6 {
    struct EpochIntegrationBundle;
    struct IslaEpochResult;
}

namespace ailee::semantics {

struct CanonicalMetadata {
    uint32_t protocol_version;
    uint64_t epoch_id;
    std::string state_root_hash;
    l6::ZKBackendType backend_type;
    bool proof_attached;
    double coherence_score;
};

CanonicalMetadata encode_metadata(const l6::EpochIntegrationBundle& bundle, const l6::IslaEpochResult& result, double coherence_score, uint32_t protocol_version);
std::array<uint8_t, 32> hash_canonical_metadata(const CanonicalMetadata& metadata);

} // namespace ailee::semantics
