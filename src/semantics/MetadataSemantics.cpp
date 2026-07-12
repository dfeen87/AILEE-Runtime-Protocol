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
    meta.backend_type = l6::ZKBackendType::MOCK;
    meta.proof_attached = result.zk_result.should_attach_proof && (result.zk_result.anchor_payload.zk_metadata.validation_status == l6::DeterministicZKStatus::OK);
    meta.coherence_score = coherence_score;
    meta.has_validity_surface = false;
    return meta;
}

CanonicalMetadata encode_metadata_v29(const l6::EpochIntegrationBundle& bundle, const l6::IslaEpochResult& result, double coherence_score, uint32_t protocol_version, const l6::auditor::ZkEpochValiditySurface& surface) {
    CanonicalMetadata meta = encode_metadata(bundle, result, coherence_score, protocol_version);
    meta.has_validity_surface = true;
    meta.validity_surface = surface;
    return meta;
}

CanonicalMetadata encode_metadata_v30(const l6::EpochIntegrationBundle& bundle, const l6::IslaEpochResult& result, double coherence_score, uint32_t protocol_version, const l6::auditor::ZkEpochValiditySurface& validity_surface, const EnergyResilienceSurface& energy_surface) {
    CanonicalMetadata meta = encode_metadata_v29(bundle, result, coherence_score, protocol_version, validity_surface);
    meta.has_energy_resilience_surface = true;
    meta.energy_resilience_surface = energy_surface;
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

    if (metadata.has_validity_surface) {
        // Encode a deterministic representation of the validity surface
        uint64_t drift = static_cast<uint64_t>(metadata.validity_surface.coherence.drift * 1000000.0);
        uint64_t stability = static_cast<uint64_t>(metadata.validity_surface.coherence.stability * 1000000.0);
        uint64_t anchor_coh = static_cast<uint64_t>(metadata.validity_surface.anchor.anchor_coherence * 1000000.0);

        ss << "|" << drift << "|" << stability << "|" << anchor_coh;
    }

    if (metadata.has_energy_resilience_surface) {
        // Encode a deterministic representation of the energy surface
        int64_t e_drift = static_cast<int64_t>(metadata.energy_resilience_surface.energy_drift * 1000000.0);
        uint64_t e_stability = static_cast<uint64_t>(metadata.energy_resilience_surface.energy_stability * 1000000.0);
        uint64_t e_cost = static_cast<uint64_t>(metadata.energy_resilience_surface.energy_cost_of_drift * 1000000.0);
        uint64_t e_anchor = static_cast<uint64_t>(metadata.energy_resilience_surface.energy_anchor_coherence * 1000000.0);
        uint64_t e_predictive = static_cast<uint64_t>(metadata.energy_resilience_surface.energy_predictive_score * 1000000.0);

        ss << "|" << e_drift << "|" << e_stability << "|" << e_cost << "|" << e_anchor << "|" << e_predictive;
    }

    std::string data = ss.str();

    std::array<uint8_t, 32> hash;
    SHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hash.data());
    return hash;
}

} // namespace ailee::semantics
