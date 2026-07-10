#include <gtest/gtest.h>
#include "l4/ReplayTick.h"

using namespace ailee::l4;

TEST(DeterministicCompressionTest, ReplayTickSerializationRoundTrip) {
    ReplayTick original = {};

    original.scheduler_state.tick_count = 42;
    original.view.total_nodes = 3;
    original.view.total_steps = 100;

    ClusterNodeState node = {};

    new (&node.peer_sync_states) std::vector<ailee::l3::PeerSyncState>();

    node.step_counter = 10;
    ailee::l3::PeerSyncState sync;
    sync.coherence_delta = 12345;
    node.peer_sync_states.push_back(sync);
    original.view.nodes.push_back(node);

    auto serialized = original.serialize();
    auto deserialized = ReplayTick::deserialize(serialized);

    EXPECT_EQ(deserialized.scheduler_state.tick_count, 42);
    EXPECT_EQ(deserialized.view.total_nodes, 3);
    EXPECT_EQ(deserialized.view.total_steps, 100);
    ASSERT_EQ(deserialized.view.nodes.size(), 1);
    EXPECT_EQ(deserialized.view.nodes[0].step_counter, 10);
    ASSERT_EQ(deserialized.view.nodes[0].peer_sync_states.size(), 1);
    EXPECT_EQ(deserialized.view.nodes[0].peer_sync_states[0].coherence_delta, 12345);
}

#include "l5/DeterministicCompressor.h"

TEST(DeterministicCompressionTest, DeltaEncoding) {
    ailee::l5::CompressionConfig config;
    config.enable_delta = true;
    config.enable_rle = false;
    config.enable_stable_hash = false;
    config.max_compressed_bytes = 100000;

    ailee::l5::DeterministicCompressor compressor(config);

    ReplayTick tick1, tick2;
    tick1.scheduler_state.tick_count = 1;
    tick2.scheduler_state.tick_count = 2;

    auto comp1 = compressor.compress_tick(tick1.view, tick1);
    auto comp2 = compressor.compress_tick(tick2.view, tick2); // This should be delta encoded against tick1

    ailee::l5::DeterministicCompressor decompressor(config);
    auto decomp1 = decompressor.decompress_tick(comp1);
    auto decomp2 = decompressor.decompress_tick(comp2);

    EXPECT_EQ(decomp1.scheduler_state.tick_count, 1);
    EXPECT_EQ(decomp2.scheduler_state.tick_count, 2);
}

TEST(DeterministicCompressionTest, RleEncoding) {
    ailee::l5::CompressionConfig config;
    config.enable_delta = false;
    config.enable_rle = true;
    config.enable_stable_hash = false;
    config.max_compressed_bytes = 100000;

    ailee::l5::DeterministicCompressor compressor(config);

    ReplayTick tick;
    tick.scheduler_state.tick_count = 42;
    // Pad with lots of identical bytes to test RLE (zeros via default init)
    tick.view.nodes.resize(100);

    auto comp = compressor.compress_tick(tick.view, tick);

    ailee::l5::DeterministicCompressor decompressor(config);
    auto decomp = decompressor.decompress_tick(comp);

    EXPECT_EQ(decomp.scheduler_state.tick_count, 42);
    EXPECT_EQ(decomp.view.nodes.size(), 100);
}

TEST(DeterministicCompressionTest, StableHashing) {
    ailee::l5::CompressionConfig config;
    config.enable_delta = false;
    config.enable_rle = false;
    config.enable_stable_hash = true;
    config.max_compressed_bytes = 100000;

    ailee::l5::DeterministicCompressor compressor(config);

    ReplayTick tick;
    tick.scheduler_state.tick_count = 99;

    auto comp = compressor.compress_tick(tick.view, tick);

    ailee::l5::DeterministicCompressor decompressor(config);
    auto decomp = decompressor.decompress_tick(comp);

    EXPECT_EQ(decomp.scheduler_state.tick_count, 99);

    // Test tampering
    comp[0] ^= 0xFF; // Modify a byte

    bool threw = false;
    try {
        decompressor.decompress_tick(comp);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST(DeterministicCompressionTest, EndToEndRoundTrip) {
    ailee::l5::DeterministicCompressor compressor; // Uses default config

    ReplayTick tick1, tick2;
    tick1.scheduler_state.tick_count = 1;
    tick1.view.total_nodes = 3;

    tick2.scheduler_state.tick_count = 2;
    tick2.view.total_nodes = 4;

    auto comp1 = compressor.compress_tick(tick1.view, tick1);
    auto comp2 = compressor.compress_tick(tick2.view, tick2);

    ailee::l5::DeterministicCompressor decompressor;
    auto decomp1 = decompressor.decompress_tick(comp1);
    auto decomp2 = decompressor.decompress_tick(comp2);

    EXPECT_EQ(decomp1.scheduler_state.tick_count, 1);
    EXPECT_EQ(decomp1.view.total_nodes, 3);

    EXPECT_EQ(decomp2.scheduler_state.tick_count, 2);
    EXPECT_EQ(decomp2.view.total_nodes, 4);
}
