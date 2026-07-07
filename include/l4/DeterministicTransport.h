#pragma once

#include <cstdint>
#include <vector>

namespace ailee {
namespace l4 {

struct alignas(64) TransportMessage {
    uint64_t source_node_id_hash;
    uint64_t dest_node_id_hash;
    uint64_t epoch_height;
    uint8_t message_type;
    // 0 = ENVELOPE
    // 1 = STATE_ROOT_ANNOUNCEMENT
    // 2 = MESH_ANCHOR
    uint8_t payload[128];
    uint8_t message_hash[32];
    uint8_t padding[7]; // to reach 192 bytes
};
static_assert(sizeof(TransportMessage) == 192, "TransportMessage must be 192 bytes");

struct alignas(64) TransportQueue {
    std::vector<TransportMessage> messages; // 24 bytes
    uint8_t padding[40]; // 24 + 40 = 64
};
static_assert(sizeof(TransportQueue) == 64, "TransportQueue must be 64 bytes");

void enqueue_transport_message(
    TransportQueue& queue,
    const TransportMessage& msg);

std::vector<TransportMessage> drain_transport_messages_for_node(
    TransportQueue& queue,
    uint64_t dest_node_id_hash);

// Helper functions to pack payloads
void pack_envelope_payload(uint8_t payload[128], uint64_t epoch_height, const uint8_t state_root[32]);
void pack_state_root_announcement_payload(uint8_t payload[128], uint64_t epoch_height, const uint8_t state_root[32]);
void pack_mesh_anchor_payload(uint8_t payload[128], uint64_t epoch_height, const uint8_t mesh_state_root[32]);

} // namespace l4
} // namespace ailee
