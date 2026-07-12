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
#include "simulation/validation/hice_contracts.h"


namespace ailee::l6 {

class MeshCoherenceEngine;
}
#include "policy/PolicyEngine.h"
#include "l6/auditor/TemporalAuditor.h"

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
    uint64_t sequence_id; // Added for deterministic replay ordering
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

    // V28 HICE Metrics
    ailee::simulation::validation::HiceMetrics hice_metrics = {
        0.5e-6, // covariance_error (passing)
        0.5e-3, // spectral_drift
        -0.01,  // delta_memory
        0.005,  // context_leakage
        0.98,   // null_matching_rate
        0.05,   // delta_auc
        0.04,   // ci_lower_bound
        0.05    // ci_point_estimate
    };
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

    // Checks Heartbeat drift
    void check_heartbeat_drift(const ClockSnapshot& clock_state, uint64_t expected_tick_duration);

private:
    RuntimeEnvironment env_;
    std::unique_ptr<IZKProvingBackend> backend_;
    ZKBackendConfig active_config_;
    std::unique_ptr<IClock> clock_;
    std::unique_ptr<EpochScheduler> scheduler_;
    std::unique_ptr<IReplayBuffer> replay_;
    std::unique_ptr<MeshCoherenceEngine> mesh_;
    std::unique_ptr<ailee::policy::PolicyEngine> policy_;

    std::unique_ptr<auditor::TemporalMetricsBuffer> temporal_buffer_;
    std::unique_ptr<auditor::TemporalAuditor> temporal_auditor_;

    std::vector<uint64_t> recent_heartbeat_intervals_;
    uint64_t last_heartbeat_timestamp_ = 0;
};

} // namespace ailee::l6
