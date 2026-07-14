#include "l3/GossipLayer.h"
#include <cstring>
#include <openssl/sha.h>

#include "protocol/ProtocolFrame.hpp"
#include "network/MainnetDiscovery.hpp"

namespace ailee {
namespace l3 {

// ---------------------------------------------------------
// Mainnet discovery pointer + binder
// ---------------------------------------------------------
static MainnetDiscovery* g_discovery = nullptr;

void bind_mainnet_discovery_gossip(MainnetDiscovery* d) {
    g_discovery = d;
}

// ---------------------------------------------------------
// Deterministic signature verification for ProtocolFrame
// ---------------------------------------------------------
static bool verify_signature(const ProtocolFrame& pf)
{
    std::string data = pf.frame_id + pf.type + pf.version +
                       pf.node_id + std::to_string(pf.timestamp) +
                       pf.payload;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), hash);

    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(digits[(hash[i] >> 4) & 0xF]);
        hex.push_back(digits[hash[i] & 0xF]);
    }

    return (hex == pf.signature);
}

// ---------------------------------------------------------
// Build outbound gossip message
// ---------------------------------------------------------
GossipMessage build_gossip_message(
    const l2::ExecutionEnvelope& envelope,
    const mesh::MeshCoherenceResult& coherence,
    uint64_t current_sequence
) {
    GossipMessage message;
    std::memset(&message, 0, sizeof(message));

    std::memcpy(message.summary.node_identity_fingerprint, envelope.context.node_identity_hash, 32);
    std::memcpy(message.summary.state_root_hash, envelope.context.state_root_hash, 32);
    std::memcpy(message.summary.epoch_hash, envelope.context.epoch_hash, 32);
    message.summary.epoch_height = envelope.context.l1_height;
    message.summary.coherence_score = coherence.score;

    if (coherence.score == 4) {
        message.summary.flags |= GOSSIP_FLAG_HEALTHY;
    }

    message.sequence_number = current_sequence + 1;

    uint8_t buffer[sizeof(GossipSummary) + sizeof(uint64_t)];
    std::memcpy(buffer, &message.summary, sizeof(GossipSummary));
    std::memcpy(buffer + sizeof(GossipSummary), &message.sequence_number, sizeof(uint64_t));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, sizeof(buffer), message.message_hash);
#pragma GCC diagnostic pop

    return message;
}

// ---------------------------------------------------------
// Receive inbound gossip message
// ---------------------------------------------------------
GossipEnvelope receive_gossip_message(const GossipMessage& message)
{
    GossipEnvelope envelope;

    // Copy summary into remote_summary
    std::memcpy(&envelope.remote_summary,
                &message.summary,
                sizeof(GossipSummary));

    // Preserve sequence number
    envelope.received_sequence = message.sequence_number;

    // Normalize hash (for now, just copy message_hash)
    std::memcpy(envelope.normalized_hash,
                message.message_hash,
                sizeof(envelope.normalized_hash));

    // Initialize flags
    envelope.flags = 0;

    // ---------------------------------------------------------
    // MAINNET GOSSIP PROPAGATION HOOK
    // ---------------------------------------------------------
    if (g_discovery) {
        std::string peerIdHex(
            reinterpret_cast<const char*>(envelope.remote_summary.node_identity_fingerprint),
            32
        );

        if (!g_discovery->hasPeer(peerIdHex)) {
            g_discovery->addPeer(peerIdHex, "unknown");
        }
    }

    // ---------------------------------------------------------
    // Signature verification placeholder
    // ---------------------------------------------------------
    // GossipEnvelope does NOT carry ProtocolFrame anymore.
    // This block is intentionally disabled.
    //
    // envelope.flags |= GOSSIP_FLAG_VALID_SIGNATURE;

    return envelope;
}

} // namespace l3
} // namespace ailee

