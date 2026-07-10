#pragma once

#include "l6/ZKProvingBackend.h"

namespace ailee::l6 {

class ZKMockBackend : public IZKProvingBackend {
public:
    ZKProofArtifact generate_proof(
        const ZKBackendConfig& config,
        const ZKConstraintSet& constraints,
        const ZKTranscript& transcript
    ) override;

    bool verify_proof(
        const ZKBackendConfig& config,
        const ZKProofArtifact& artifact,
        const ZKConstraintSet& constraints,
        const ZKTranscript& transcript
    ) override;
};

} // namespace ailee::l6
