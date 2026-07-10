#include "l6/ZKOrchestrationManager.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace ailee::l6 {

namespace {
std::string compute_hash_hex(const std::vector<uint8_t>& data) {
    if (data.empty()) return "";
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}
} // namespace

OrchestrationResult orchestrate_epoch(
    const OrchestrationContext& ctx,
    IZKProvingBackend* backend,
    const ZKBackendConfig& backend_config,
    const ZKConstraintSet* constraints,
    const ZKTranscript* transcript,
    const std::string& state_root_hash
) {
    OrchestrationResult result;

    // 1. Determine anchoring
    bool should_anchor = (ctx.anchor_plan.decision == AnchorDecision::ANCHOR);

    // 2. Determine proof attachment
    bool scheduling_says_attach = (ctx.proof_plan.decision == ProofDecision::ATTACH_PROOF);

    ZKAnchorMetadata zk_metadata;
    std::string proof_commitment_hash = "";

    if (scheduling_says_attach && backend != nullptr) {
        if (constraints != nullptr && transcript != nullptr) {
            auto artifact = backend->generate_proof(backend_config, *constraints, *transcript);
            auto check = validate_zk_metadata(artifact.metadata, *constraints, *transcript);
            if (check.status == DeterministicZKStatus::OK && backend->verify_proof(backend_config, artifact, *constraints, *transcript)) {
                zk_metadata.constraint_set_id = artifact.metadata.constraint_set_id;
                zk_metadata.transcript_id = artifact.metadata.transcript_id;
                zk_metadata.proof_id = artifact.metadata.proof_id;
                zk_metadata.validation_status = DeterministicZKStatus::OK;
                proof_commitment_hash = compute_hash_hex(artifact.proof_bytes);
            } else {
                zk_metadata.constraint_set_id = "";
                zk_metadata.transcript_id = "";
                zk_metadata.proof_id = "";
                zk_metadata.validation_status = (check.status == DeterministicZKStatus::OK) ? DeterministicZKStatus::CONSTRAINT_MISMATCH : check.status; // Using existing error enum if verification fails to maintain backwards compatibility
            }
        } else {
            zk_metadata.constraint_set_id = "";
            zk_metadata.transcript_id = "";
            zk_metadata.proof_id = "";
            zk_metadata.validation_status = (constraints == nullptr) ? DeterministicZKStatus::EMPTY_CONSTRAINTS : DeterministicZKStatus::EMPTY_TRANSCRIPT;
        }
    } else {
        zk_metadata.constraint_set_id = "";
        zk_metadata.transcript_id = "";
        zk_metadata.proof_id = "";
        zk_metadata.validation_status = DeterministicZKStatus::OK;
    }

    // 4. Construct anchor payload
    AnchorPayload payload;
    payload.epoch_id = ctx.epoch_id;
    payload.state_root_hash = state_root_hash;
    payload.zk_metadata = zk_metadata;
    payload.proof_commitment_hash = proof_commitment_hash;

    result.anchor_payload = payload;
    result.should_anchor = should_anchor;
    result.should_attach_proof = scheduling_says_attach;

    return result;
}

} // namespace ailee::l6
