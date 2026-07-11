#include <gtest/gtest.h>
#include "l1_sync/mainnet_sync.hpp"
#include <vector>

using namespace ailee::l1_sync;

TEST(MainnetSyncMetricsTest, MetricsCalculation) {
    MainnetSyncManager manager;

    HeaderBatch headers;
    for (int i = 0; i < 10; ++i) {
        BlockHeader h;
        h.version = 1;
        h.height = 1000 + i;
        h.timestamp = 1600000000 + (i * 600); // 10 min blocks
        h.prev_hash.fill(i);
        h.hash.fill(i + 1);
        headers.push_back(h);
    }
    manager.ingest_headers(headers);

    MempoolDeltaBatch deltas;
    MempoolDelta d1;
    d1.is_add = true;
    d1.txid.fill(42);
    d1.fee = 500;
    d1.size = 250;
    deltas.push_back(d1);
    manager.ingest_mempool_deltas(deltas);

    std::string hash1 = manager.compute_utxo_reflection_hash();
    EXPECT_FALSE(hash1.empty());
    EXPECT_GE(manager.get_spectral_drift(), 0.0);

    MainnetSyncManager manager2;
    manager2.ingest_headers(headers);
    manager2.ingest_mempool_deltas(deltas);
    std::string hash2 = manager2.compute_utxo_reflection_hash();

    EXPECT_EQ(hash1, hash2);
}
