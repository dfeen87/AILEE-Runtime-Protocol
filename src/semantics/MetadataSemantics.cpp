#include "semantics/MetadataSemantics.h"
#include "l6/IslaRuntimeOrchestrator.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace ailee::semantics {

CanonicalMetadata encode_metadata(const l6::EpochIntegrationBundle& bundle, const l6::IslaEpochResult& result, double coherence_score, uint32_t protocol_version) {
    CanonicalMetadata meta;
    meta.protocol_version = protocol_version;
    meta.epoch_id = bundle.epoch_id;
    meta.state_root_hash = bundle.state_root_hash;
    // We assume backend_type is somehow available or set, but for now we'll put MOCK.
    // In integration we'll pass the active backend type.
    meta.backend_type = l6::ZKBackendType::MOCK;
    meta.proof_attached = result.zk_result.should_attach_proof && (result.zk_result.anchor_payload.zk_metadata.validation_status == l6::DeterministicZKStatus::OK);
    meta.coherence_score = coherence_score;
    return meta;
}

std::array<uint8_t, 32> hash_canonical_metadata(const CanonicalMetadata& metadata) {
    // Deterministic serialization without floats
    std::stringstream ss;
    uint64_t scaled_coherence = static_cast<uint64_t>(metadata.coherence_score * 1000000.0);
    ss << metadata.protocol_version << "|"
       << metadata.epoch_id << "|"
       << metadata.state_root_hash << "|"
       << static_cast<int>(metadata.backend_type) << "|"
       << (metadata.proof_attached ? "1" : "0") << "|"
       << scaled_coherence;

    std::string data = ss.str();

    std::array<uint8_t, 32> hash;
    SHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hash.data());
    return hash;
}

} // namespace ailee::semantics
