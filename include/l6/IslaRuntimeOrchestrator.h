#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <array>
#include <vector>

#include "l6/RuntimeEnvironment.h"
#include "l6/ZKProvingBackend.h"
#include "l6/ZKOrchestrationManager.h"
#include "l6/ProofScheduling.h"
#include "l6/DeterministicEpochAnchoring.h"
#include "l6/ZKBackendFactory.h"
#include "l6/Exceptions.h"

// Note: AnchorCommit classes are in namespace ailee::anchor
#include "anchor/AnchorCommitTxBuilder.h"
#include "anchor/AnchorCommitValidator.h"
#include "anchor/AnchorMetadataEncoder.h"
#include "l6/MeshCoherenceEngine.h"


namespace ailee::l6 {

class MeshCoherenceEngine;
}
#include "policy/PolicyEngine.h"
namespace ailee::l6 {

struct ClockSnapshot {
    uint64_t height;
    uint64_t timestamp;
    std::string hash;
    std::string source;
};

class IClock {
public:
    virtual ~IClock() = default;
    virtual ClockSnapshot get_snapshot() const = 0;
};

struct EpochIntegrationBundle {
    uint64_t epoch_id;
    std::string state_root_hash;
    AnchorPlan anchor_plan;
    ProofPlan proof_plan;
    ClockSnapshot clock_snapshot;
    SchedulerDecision scheduler_decision;

    // Anchor-commit minimal inputs
    ailee::anchor::AnchorCommitInput anchor_input;
    uint64_t fee_sats;

    // For orchestrate_epoch ZK integration:
    const ZKConstraintSet* constraints = nullptr;
    const ZKTranscript* transcript = nullptr;
};

struct IslaEpochResult {
    OrchestrationResult zk_result;
    ailee::anchor::AnchorCommitTx anchor_tx;
    ailee::anchor::AnchorCommitValidationResult validation;
    std::array<uint8_t, 32> metadata_hash;
};

class IReplayBuffer {
public:
    virtual ~IReplayBuffer() = default;
    virtual void record_epoch(const EpochIntegrationBundle& bundle,
                              const IslaEpochResult& result) = 0;

    virtual std::vector<EpochIntegrationBundle> get_epoch_history() const = 0;
    virtual void remove_oldest() = 0;
    virtual size_t size() const = 0;
    virtual size_t max_size() const = 0;
};

class IslaRuntimeOrchestrator {
public:
    explicit IslaRuntimeOrchestrator(const RuntimeEnvironment& env);

    void attach_backend(const ZKBackendConfig& config);
    // Overload for testing: attach a custom or pre-allocated backend
    void attach_backend(std::unique_ptr<IZKProvingBackend> backend, const ZKBackendConfig& config);
    // Overload for testing: attach a raw pointer (caller retains ownership)
    void attach_backend(IZKProvingBackend* backend, const ZKBackendConfig& config);

    void attach_clock(std::unique_ptr<IClock> clock);
    void attach_scheduler(std::unique_ptr<EpochScheduler> scheduler);
    void attach_replay(std::unique_ptr<IReplayBuffer> replay);
    void attach_mesh(std::unique_ptr<MeshCoherenceEngine> mesh);
    void attach_policy(std::unique_ptr<ailee::policy::PolicyEngine> policy);

    IslaEpochResult run_epoch(const EpochIntegrationBundle& bundle);

private:
    RuntimeEnvironment env_;
    std::unique_ptr<IZKProvingBackend> backend_;
    ZKBackendConfig active_config_;
    std::unique_ptr<IClock> clock_;
    std::unique_ptr<EpochScheduler> scheduler_;
    std::unique_ptr<IReplayBuffer> replay_;
    std::unique_ptr<MeshCoherenceEngine> mesh_;
    std::unique_ptr<ailee::policy::PolicyEngine> policy_;
};

} // namespace ailee::l6
