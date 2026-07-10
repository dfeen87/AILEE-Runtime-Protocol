#include "l6/ZKDeterministicValidator.h"

namespace ailee::l6 {

DeterministicZKCheck validate_zk_metadata(
    const ZKProofMetadata& proof,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    DeterministicZKCheck result;
    result.proof_id = proof.proof_id;
    result.constraint_set_id = proof.constraint_set_id;
    result.transcript_id = proof.transcript_id;

    if (constraints.num_constraints == 0) {
        result.status = DeterministicZKStatus::EMPTY_CONSTRAINTS;
        return result;
    }

    if (transcript.num_rounds == 0) {
        result.status = DeterministicZKStatus::EMPTY_TRANSCRIPT;
        return result;
    }

    if (proof.constraint_set_id != constraints.id) {
        result.status = DeterministicZKStatus::CONSTRAINT_MISMATCH;
        return result;
    }

    if (proof.transcript_id != transcript.id) {
        result.status = DeterministicZKStatus::TRANSCRIPT_MISMATCH;
        return result;
    }

    result.status = DeterministicZKStatus::OK;
    return result;
}

} // namespace ailee::l6
