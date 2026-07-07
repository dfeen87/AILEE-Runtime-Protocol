#include "l4/DeterministicTransport.h"
#include <openssl/sha.h>
#include <algorithm>
#include <cstring>

namespace ailee {
namespace l4 {

namespace {

// Helper to serialize uint64_t into a byte array in little-endian format
void serialize_uint64_le(uint64_t val, uint8_t out[8]) {
    out[0] = static_cast<uint8_t>(val & 0xFF);
    out[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    out[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    out[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    out[4] = static_cast<uint8_t>((val >> 32) & 0xFF);
    out[5] = static_cast<uint8_t>((val >> 40) & 0xFF);
    out[6] = static_cast<uint8_t>((val >> 48) & 0xFF);
    out[7] = static_cast<uint8_t>((val >> 56) & 0xFF);
}

} // anonymous namespace

void enqueue_transport_message(
    TransportQueue& queue,
    const TransportMessage& msg) {

    TransportMessage new_msg = msg;

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    uint8_t tmp[8];
    serialize_uint64_le(new_msg.source_node_id_hash, tmp);
    SHA256_Update(&ctx, tmp, 8);

    serialize_uint64_le(new_msg.dest_node_id_hash, tmp);
    SHA256_Update(&ctx, tmp, 8);

    serialize_uint64_le(new_msg.epoch_height, tmp);
    SHA256_Update(&ctx, tmp, 8);

    SHA256_Update(&ctx, &new_msg.message_type, 1);
    SHA256_Update(&ctx, new_msg.payload, 128);

    SHA256_Final(new_msg.message_hash, &ctx);

    queue.messages.push_back(new_msg);
}

std::vector<TransportMessage> drain_transport_messages_for_node(
    TransportQueue& queue,
    uint64_t dest_node_id_hash) {

    std::vector<TransportMessage> drained;
    std::vector<TransportMessage> remaining;

    for (const auto& msg : queue.messages) {
        if (msg.dest_node_id_hash == dest_node_id_hash) {
            drained.push_back(msg);
        } else {
            remaining.push_back(msg);
        }
    }

    queue.messages = std::move(remaining);

    std::sort(drained.begin(), drained.end(), [](const TransportMessage& a, const TransportMessage& b) {
        if (a.epoch_height != b.epoch_height) {
            return a.epoch_height < b.epoch_height;
        }
        if (a.source_node_id_hash != b.source_node_id_hash) {
            return a.source_node_id_hash < b.source_node_id_hash;
        }
        return a.message_type < b.message_type;
    });

    return drained;
}

void pack_envelope_payload(uint8_t payload[128], uint64_t epoch_height, const uint8_t state_root[32]) {
    std::memset(payload, 0, 128);
    serialize_uint64_le(epoch_height, payload);
    std::memcpy(payload + 8, state_root, 32);
}

void pack_state_root_announcement_payload(uint8_t payload[128], uint64_t epoch_height, const uint8_t state_root[32]) {
    std::memset(payload, 0, 128);
    serialize_uint64_le(epoch_height, payload);
    std::memcpy(payload + 8, state_root, 32);
}

void pack_mesh_anchor_payload(uint8_t payload[128], uint64_t epoch_height, const uint8_t mesh_state_root[32]) {
    std::memset(payload, 0, 128);
    serialize_uint64_le(epoch_height, payload);
    std::memcpy(payload + 8, mesh_state_root, 32);
}

} // namespace l4
} // namespace ailee
