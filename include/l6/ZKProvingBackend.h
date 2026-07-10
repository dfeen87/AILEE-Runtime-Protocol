#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "l6/ZKMetadata.h"

namespace ailee::l6 {

enum class ZKBackendType {
    MOCK,          // existing deterministic backend
    HALO2_NATIVE,  // stub Halo2 backend
    PLONK_NATIVE   // stub Plonk backend
};

struct ZKBackendConfig {
    ZKBackendType type;
    std::string circuit_id; // deterministic circuit identifier
    std::string circuit_path;          // path to circuit artifact (stub for now)
    std::string proving_key_path;      // path to proving key (stub for now)
    std::string verification_key_path; // path to verification key (stub for now)
};

struct ZKProofArtifact {
    ZKProofMetadata metadata; // protocol-level metadata
    std::vector<uint8_t> proof_bytes; // opaque proof bytes from backend
};

class IZKProvingBackend {
public:
    virtual ~IZKProvingBackend() = default;

    virtual ZKProofArtifact generate_proof(
        const ZKBackendConfig& config,
        const ZKConstraintSet& constraints,
        const ZKTranscript& transcript
    ) = 0;

    virtual bool verify_proof(
        const ZKBackendConfig& config,
        const ZKProofArtifact& artifact,
        const ZKConstraintSet& constraints,
        const ZKTranscript& transcript
    ) = 0;
};

} // namespace ailee::l6
