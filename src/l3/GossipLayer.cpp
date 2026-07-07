#include "l3/GossipLayer.h"
#include <cstring>
#include <openssl/sha.h>

namespace ailee {
namespace l3 {

GossipMessage build_gossip_message(
    const l2::ExecutionEnvelope& envelope,
    const mesh::MeshCoherenceResult& coherence,
    uint64_t current_sequence
) {
    GossipMessage message;
    std::memset(&message, 0, sizeof(message));

    // Fill GossipSummary
    std::memcpy(message.summary.node_identity_fingerprint, envelope.context.node_identity_hash, 32);
    std::memcpy(message.summary.state_root_hash, envelope.context.state_root_hash, 32);
    std::memcpy(message.summary.epoch_hash, envelope.context.epoch_hash, 32);
    message.summary.epoch_height = envelope.context.l1_height;
    message.summary.coherence_score = coherence.score;

    // Set healthy flag if coherence score is fully coherent (4)
    if (coherence.score == 4) {
        message.summary.flags |= GOSSIP_FLAG_HEALTHY;
    }

    message.sequence_number = current_sequence + 1;

    // Compute deterministic message hash
    // We hash the summary and the sequence number
    uint8_t buffer[sizeof(GossipSummary) + sizeof(uint64_t)];
    std::memcpy(buffer, &message.summary, sizeof(GossipSummary));
    std::memcpy(buffer + sizeof(GossipSummary), &message.sequence_number, sizeof(uint64_t));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, sizeof(buffer), message.message_hash);
#pragma GCC diagnostic pop

    return message;
}

GossipEnvelope receive_gossip_message(const GossipMessage& message) {
    GossipEnvelope envelope;
    std::memset(&envelope, 0, sizeof(envelope));

    std::memcpy(&envelope.remote_summary, &message.summary, sizeof(GossipSummary));
    envelope.received_sequence = message.sequence_number;

    // Normalization logic: compute a normalized hash purely from the remote summary
    // Since we ignore the sequence number for the normalized hash (which represents the state),
    // it helps identify nodes broadcasting the same state multiple times.
    uint8_t buffer[sizeof(GossipSummary)];
    std::memcpy(buffer, &envelope.remote_summary, sizeof(GossipSummary));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, sizeof(buffer), envelope.normalized_hash);
#pragma GCC diagnostic pop

    return envelope;
}

} // namespace l3
} // namespace ailee
