#include <gtest/gtest.h>
#include "l4/DeterministicTransport.h"
#include <cstring>
#include <vector>
#include <openssl/sha.h>

using namespace ailee::l4;

class DeterministicTransportTest : public ::testing::Test {
protected:
    TransportQueue queue;

public:
    void SetUp() override {
        queue.messages.clear();
        std::memset(queue.padding, 0, sizeof(queue.padding));
    }
};

TEST_F(DeterministicTransportTest, EnqueueAndHashStability) {
    TransportMessage msg1 = {};
    msg1.source_node_id_hash = 100;
    msg1.dest_node_id_hash = 200;
    msg1.epoch_height = 42;
    msg1.message_type = 0; // ENVELOPE
    std::memset(msg1.payload, 0xAA, 128);

    enqueue_transport_message(queue, msg1);

    ASSERT_EQ(queue.messages.size(), 1);

    // Check that hash is not all zeros
    bool all_zeros = true;
    for (int i = 0; i < 32; ++i) {
        if (queue.messages[0].message_hash[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    EXPECT_FALSE(all_zeros);

    // Enqueue identical message, should have identical hash
    enqueue_transport_message(queue, msg1);
    ASSERT_EQ(queue.messages.size(), 2);
    EXPECT_EQ(std::memcmp(queue.messages[0].message_hash, queue.messages[1].message_hash, 32), 0);

    // Enqueue changed message, should have different hash
    TransportMessage msg2 = msg1;
    msg2.epoch_height = 43;
    enqueue_transport_message(queue, msg2);
    ASSERT_EQ(queue.messages.size(), 3);
    EXPECT_NE(std::memcmp(queue.messages[0].message_hash, queue.messages[2].message_hash, 32), 0);
}

TEST_F(DeterministicTransportTest, DrainAndDeterministicOrdering) {
    TransportMessage msg1 = {};
    msg1.source_node_id_hash = 300;
    msg1.dest_node_id_hash = 100;
    msg1.epoch_height = 11;
    msg1.message_type = 1;

    TransportMessage msg2 = {};
    msg2.source_node_id_hash = 200;
    msg2.dest_node_id_hash = 100;
    msg2.epoch_height = 10;
    msg2.message_type = 0;

    TransportMessage msg3 = {};
    msg3.source_node_id_hash = 200;
    msg3.dest_node_id_hash = 100;
    msg3.epoch_height = 11;
    msg3.message_type = 0;

    // Enqueue in random order
    enqueue_transport_message(queue, msg1); // h:11, s:300, t:1
    enqueue_transport_message(queue, msg3); // h:11, s:200, t:0
    enqueue_transport_message(queue, msg2); // h:10, s:200, t:0

    auto drained = drain_transport_messages_for_node(queue, 100);

    ASSERT_EQ(drained.size(), 3);
    EXPECT_EQ(queue.messages.size(), 0);

    // Expected sorting:
    // 1. epoch_height (msg2: 10, msg3: 11, msg1: 11)
    // 2. source_node_id_hash (msg3: 200, msg1: 300)
    // 3. message_type

    EXPECT_EQ(drained[0].epoch_height, 10);
    EXPECT_EQ(drained[0].source_node_id_hash, 200);

    EXPECT_EQ(drained[1].epoch_height, 11);
    EXPECT_EQ(drained[1].source_node_id_hash, 200);

    EXPECT_EQ(drained[2].epoch_height, 11);
    EXPECT_EQ(drained[2].source_node_id_hash, 300);
}

TEST_F(DeterministicTransportTest, MultiNodeRouting) {
    TransportMessage msg1 = {};
    msg1.dest_node_id_hash = 100;

    TransportMessage msg2 = {};
    msg2.dest_node_id_hash = 200;

    TransportMessage msg3 = {};
    msg3.dest_node_id_hash = 100;

    enqueue_transport_message(queue, msg1);
    enqueue_transport_message(queue, msg2);
    enqueue_transport_message(queue, msg3);

    auto drained_100 = drain_transport_messages_for_node(queue, 100);
    ASSERT_EQ(drained_100.size(), 2);
    ASSERT_EQ(queue.messages.size(), 1); // Only node 200 message left

    auto drained_200 = drain_transport_messages_for_node(queue, 200);
    ASSERT_EQ(drained_200.size(), 1);
    ASSERT_EQ(queue.messages.size(), 0);
}

TEST_F(DeterministicTransportTest, PayloadIntegrity) {
    uint8_t payload[128];
    std::memset(payload, 0, sizeof(payload));
    uint64_t epoch = 123456789;
    uint8_t state_root[32];
    std::memset(state_root, 0, sizeof(state_root));
    for (int i = 0; i < 32; ++i) state_root[i] = static_cast<uint8_t>(i);

    pack_envelope_payload(payload, epoch, state_root);

    TransportMessage msg = {};
    msg.dest_node_id_hash = 100;
    std::memcpy(msg.payload, payload, 128);

    enqueue_transport_message(queue, msg);

    auto drained = drain_transport_messages_for_node(queue, 100);
    ASSERT_EQ(drained.size(), 1);

    // Decode and verify
    const uint8_t* drained_payload = drained[0].payload;

    // Read epoch_height (LE)
    uint64_t decoded_epoch = 0;
    for (int i = 0; i < 8; ++i) {
        decoded_epoch |= (static_cast<uint64_t>(drained_payload[i]) << (i * 8));
    }

    EXPECT_EQ(decoded_epoch, epoch);

    // Read state_root
    EXPECT_EQ(std::memcmp(drained_payload + 8, state_root, 32), 0);

    // Verify remaining bytes are zero
    bool remaining_zeros = true;
    for (int i = 40; i < 128; ++i) {
        if (drained_payload[i] != 0) remaining_zeros = false;
    }
    EXPECT_TRUE(remaining_zeros);
}
