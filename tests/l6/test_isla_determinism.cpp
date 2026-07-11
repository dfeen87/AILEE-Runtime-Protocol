#include <gtest/gtest.h>
#include "l6/IslaRuntimeOrchestrator.h"
#include "l6/RuntimeEnvironment.h"
#include "l6/ZKMockBackend.h"

using namespace ailee::l6;

class DeterministicClockStub : public IClock {
public:
    ClockSnapshot get_snapshot() const override {
        return {1000, 1600000000, "0000000000000000000000000000000000000000000000000000000000000000", "mock"};
    }
};

class DeterministicSchedulerStub : public EpochScheduler {
public:
    SchedulerDecision get_decision([[maybe_unused]] uint64_t epoch_id) const override {
        return {AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF};
    }
};

class ReplayStub : public IReplayBuffer {
public:
    void record_epoch([[maybe_unused]] const EpochIntegrationBundle& bundle, [[maybe_unused]] const IslaEpochResult& result) override {}
    std::vector<EpochIntegrationBundle> get_epoch_history() const override { return {}; }
    void remove_oldest() override {}
    size_t size() const override { return 0; }
    size_t max_size() const override { return 1000; }
};

TEST(IslaDeterminism, SameBundleProducesSameResult) {
    RuntimeEnvironment env;
    env.is_ci = true;

    IslaRuntimeOrchestrator isla(env);

    ZKBackendConfig mock_config{ZKBackendType::MOCK, "determinism_circuit", "", "", ""};
    isla.attach_backend(mock_config);
    isla.attach_clock(std::make_unique<DeterministicClockStub>());
    isla.attach_scheduler(std::make_unique<DeterministicSchedulerStub>());
    isla.attach_replay(std::make_unique<ReplayStub>());

    ZKConstraintSet constraints{"constraints_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    EpochIntegrationBundle bundle{};
    bundle.epoch_id = 42;
    bundle.state_root_hash = "deadbeef1234567890abcdef1234567890abcdef1234567890abcdef12345678"; // 64 chars
    bundle.anchor_input.internal_key.fill(1);
    bundle.clock_snapshot = {1000, 1600000000, "0000000000000000000000000000000000000000000000000000000000000000", "mock"};
    bundle.anchor_input.prev_txid.fill(2);
    bundle.anchor_input.prev_vout = 0;
    bundle.anchor_input.value_sats = 10000;
    bundle.fee_sats = 500;
    bundle.constraints = &constraints;
    bundle.transcript = &transcript;

    // Explicitly set plan to match stub otherwise orchestrator might not overwrite safely in tests if scheduler is used
    // Actually the Isla implementation calls scheduler_->get_decision() and overwrites the decision flags.

    auto r1 = isla.run_epoch(bundle);
    auto r2 = isla.run_epoch(bundle);

    ASSERT_EQ(r1.zk_result.anchor_payload.proof_commitment_hash, r2.zk_result.anchor_payload.proof_commitment_hash);
    ASSERT_EQ(r1.metadata_hash, r2.metadata_hash);
    ASSERT_EQ(r1.anchor_tx.raw_tx, r2.anchor_tx.raw_tx);
    ASSERT_EQ(r1.validation.ok, r2.validation.ok);
}
