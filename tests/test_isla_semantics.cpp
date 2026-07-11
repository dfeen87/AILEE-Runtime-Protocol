#include <gtest/gtest.h>
#include "semantics/SemanticsTypes.h"
#include "semantics/EpochSemantics.h"
#include "semantics/ClockSemantics.h"
#include "semantics/MeshCoherenceSemantics.h"
#include "semantics/AttachmentSemantics.h"
#include "semantics/ReplaySemantics.h"
#include "semantics/MetadataSemantics.h"
#include "l6/IslaRuntimeOrchestrator.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

using namespace ailee::semantics;
using namespace ailee::l6;

class MockReplayBuffer : public IReplayBuffer {
public:
    std::vector<EpochIntegrationBundle> history;
    void record_epoch(const EpochIntegrationBundle& bundle, const IslaEpochResult& result) override {
        history.push_back(bundle);
    }
    std::vector<EpochIntegrationBundle> get_epoch_history() const override {
        return history;
    }
    void remove_oldest() override {
        if (!history.empty()) history.erase(history.begin());
    }
    size_t size() const override { return history.size(); }
    size_t max_size() const override { return 100; }
};

TEST(IslaSemanticsTest, MeshCoherence) {
    MeshState state;
    state.node_count = 0;
    EXPECT_DOUBLE_EQ(MeshCoherenceSemantics::compute_coherence_score(state), 0.0);

    state.node_count = 10;
    state.active_links = 5; // density 0.5 -> * 0.5 = 0.25
    state.latency_variance = 0.5; // (1 - 0.5) * 0.3 = 0.15
    state.partition_count = 1; // ratio 0.1 -> (1 - 0.1) * 0.2 = 0.18
    // Total score = 0.25 + 0.15 + 0.18 = 0.58
    EXPECT_NEAR(MeshCoherenceSemantics::compute_coherence_score(state), 0.58, 0.001);
}

TEST(IslaSemanticsTest, EpochValidation) {
    MockReplayBuffer replay;
    EpochIntegrationBundle bundle;

    ZKConstraintSet constraints{"c1", 1};
    ZKTranscript transcript{"t1", 1};
    bundle.constraints = &constraints;
    bundle.transcript = &transcript;

    std::vector<uint8_t> t_bytes = transcript.to_bytes();
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(t_bytes.data(), t_bytes.size(), hash.data());
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    bundle.state_root_hash = ss.str();

    bundle.clock_snapshot.height = 1000;
    bundle.clock_snapshot.timestamp = 100000;
    bundle.epoch_id = 1; // 1000 / 1000

    EXPECT_TRUE(EpochSemantics::validate_epoch_bundle(bundle, replay, 1000));
}

TEST(IslaSemanticsTest, ClockValidation) {
    MockReplayBuffer replay;
    ClockSnapshot snap;
    snap.timestamp = 1000;
    snap.height = 1000;
    snap.hash = std::string(64, 'a');
    snap.source = "Bitcoin";

    EXPECT_TRUE(ClockSemantics::is_valid_snapshot(snap, replay, EnvironmentType::PROD));

    snap.hash = "short";
    EXPECT_FALSE(ClockSemantics::is_valid_snapshot(snap, replay, EnvironmentType::PROD));
}
