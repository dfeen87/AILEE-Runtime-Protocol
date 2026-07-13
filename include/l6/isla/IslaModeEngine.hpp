#pragma once

#include "l6/isla/IslaMetrics.hpp"
#include "l6/isla/IslaTuningDecision.hpp"

namespace ailee::l6::isla {

class IslaModeEngine {
public:
    IslaTuningDecision computeDecision(
        const EpochMetricsWindow& epoch_metrics,
        const PerformanceMetricsWindow& perf_metrics,
        const EconomicMetricsWindow& econ_metrics,
        const IslaTuningDecision& current_state
    ) const;

    // Pure deterministic score functions exposed for testing
    static double computeCoherenceScore(const EpochMetricsWindow& window);
    static double computePerformanceScore(const PerformanceMetricsWindow& window);
    static double computeEconomicScore(const EconomicMetricsWindow& window);

private:
    static constexpr uint32_t MIN_BATCH_SIZE = 1;
    static constexpr uint32_t MAX_BATCH_SIZE = 1024;

    static constexpr uint32_t MIN_PROOF_INTERVAL_MS = 250;
    static constexpr uint32_t MAX_PROOF_INTERVAL_MS = 5000;

    static constexpr uint32_t MIN_WORKER_ALLOCATION = 1;
    static constexpr uint32_t MAX_WORKER_ALLOCATION = 64; // arbitrary large number for deterministic hardware fallback max limit

};

} // namespace ailee::l6::isla
