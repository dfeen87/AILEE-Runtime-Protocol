// AdapterRegistryTests.cpp
// Unit tests for AILEE‑Core Global_Seven AdapterRegistry
// Requires GoogleTest (gtest) framework

#include "Global_Seven.h"
#include <gtest/gtest.h>

using namespace ailee::global_seven;

// Dummy adapter for testing
class DummyAdapter : public IChainAdapter {
public:
    bool init(const AdapterConfig& /*cfg*/, ErrorCallback /*onError*/) override { return true; }
    bool start(TxCallback /*onTx*/, BlockCallback /*onBlock*/, EnergyCallback /*onEnergy*/) override { return true; }
    void stop() override {}
    bool broadcastTransaction(const std::vector<TxOut>& /*outputs*/,
                              const std::unordered_map<std::string, std::string>& /*metadata*/,
                              std::string& outChainTxId) override {
        outChainTxId = "dummy_txid";
        return true;
    }
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override {
        NormalizedTx nt;
        nt.chainTxId = chainTxId;
        nt.chain = Chain::Custom1;
        return nt;
    }
    std::optional<BlockHeader> getBlockHeader(const std::string& blockHash) override {
        BlockHeader bh;
        bh.hash = blockHash;
        bh.chain = Chain::Custom1;
        return bh;
    }
    std::optional<uint64_t> getBlockHeight() override { return 42ULL; }
    Chain chain() const override { return Chain::Custom1; }
    AdapterTraits traits() const override {
        return AdapterTraits{true,true,false,false,false,
                             UnitSpec{8,"sats","DUM"}, "DummyAdapter","0.1.0",true};
    }
};

// ---- Tests ----

TEST(AdapterRegistryTest, RegisterAndRetrieveAdapter) {
    auto& registry = AdapterRegistry::instance();

    // Register dummy adapter
    registry.registerAdapter(Chain::Custom1, std::unique_ptr<IChainAdapter>(new DummyAdapter()));

    // Retrieve adapter
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    // Verify traits
    AdapterTraits traits = adapter->traits();
    EXPECT_EQ(traits.adapterName, "DummyAdapter");
    EXPECT_TRUE(traits.audited);
}

TEST(AdapterRegistryTest, BroadcastTransactionWorks) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    std::string txid;
    bool ok = adapter->broadcastTransaction({}, {}, txid);
    EXPECT_TRUE(ok);
    EXPECT_EQ(txid, "dummy_txid");
}

TEST(AdapterRegistryTest, GetTransactionReturnsNormalizedTx) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    auto tx = adapter->getTransaction("abc123");
    ASSERT_TRUE(tx.has_value());
    EXPECT_EQ(tx->chainTxId, "abc123");
    EXPECT_EQ(tx->chain, Chain::Custom1);
}

TEST(AdapterRegistryTest, GetBlockHeaderReturnsHeader) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    auto bh = adapter->getBlockHeader("blockhash");
    ASSERT_TRUE(bh.has_value());
    EXPECT_EQ(bh->hash, "blockhash");
    EXPECT_EQ(bh->chain, Chain::Custom1);
}

TEST(AdapterRegistryTest, GetBlockHeightReturnsValue) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    auto h = adapter->getBlockHeight();
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h.value(), 42ULL);
}

TEST(AdapterConfigTest, ReadOnlyDefaultIsTrue) {
    AdapterConfig cfg;
    EXPECT_TRUE(cfg.readOnly);
}

#if defined(AILEE_BITCOIN_WRITE_DISABLED)
TEST(BitcoinShadowModeTest, WriteDisabledAtCompileTime) {
    // When AILEE_BITCOIN_WRITE_DISABLED is set, the macro is active.
    // This test documents that the compile-time gate is in effect.
    EXPECT_TRUE(true);
}
#else
TEST(BitcoinShadowModeTest, WriteEnabledAtCompileTime) {
    // AILEE_BITCOIN_WRITE_ENABLED=ON was set; write ops are compiled in.
    EXPECT_TRUE(true);
}
#endif

TEST(AnchorCommitmentTest, ComputeTweakedKeyWithValidInput) {
    AnchorCommitment commitment;
    commitment.commitmentBytes = std::vector<uint8_t>(96, 0x01); // Fake commitment

    // Valid secp256k1 pubkey (example)
    std::vector<uint8_t> internalPubkey = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };

    EXPECT_TRUE(commitment.computeTweakedKey(internalPubkey));
    EXPECT_EQ(commitment.tweakedTaprootKey.size(), 32);

    EXPECT_TRUE(AnchorCommitment::validateTaprootCommitment(commitment.tweakedTaprootKey, internalPubkey, commitment.commitmentBytes));
}

TEST(AnchorCommitmentTest, BuildTaprootCommitmentKeyPath) {
    AnchorCommitment commitment;
    commitment.commitmentBytes = std::vector<uint8_t>(96, 0x01); // Fake commitment

    // Valid secp256k1 pubkey (example)
    std::vector<uint8_t> internalPubkey = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };

    EXPECT_TRUE(commitment.computeTweakedKey(internalPubkey));
    EXPECT_EQ(commitment.tweakedTaprootKey.size(), 32);

    auto taprootPayload = commitment.buildTaprootCommitment();
    EXPECT_EQ(taprootPayload.scriptBytes.size(), 34); // OP_1 (1) + OP_PUSH32 (1) + 32 bytes = 34
    EXPECT_EQ(taprootPayload.scriptBytes[0], 0x51); // OP_1
    EXPECT_EQ(taprootPayload.scriptBytes[1], 0x20); // Push 32 bytes
    EXPECT_TRUE(taprootPayload.description.find("TAPROOT_KEY_PATH") != std::string::npos);
}
