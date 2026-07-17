#include "l6/isla/IslaModeEngine.hpp"
#include <algorithm>
#include <cmath>

namespace ailee::l6::isla {

double IslaModeEngine::computeCoherenceScore(const EpochMetricsWindow& window) {
    if (window.epochs.empty()) return 1.0;

    double total_score = 0.0;
    for (const auto& epoch : window.epochs) {
        double score = 1.0;

        // dispute_rate is bad, subtract heavily
        score -= (epoch.dispute_rate * 0.5);

        // reorgs are very bad
        if (epoch.reorg_alerts > 0) {
            score -= (epoch.reorg_alerts * 0.2);
        }

        // latency penalty
        if (epoch.finalization_latency > 1000.0) {
            score -= 0.1;
        }

        if (score < 0.0) score = 0.0;
        total_score += score;
    }

    return std::clamp(total_score / static_cast<double>(window.epochs.size()), 0.0, 1.0);
}

double IslaModeEngine::computePerformanceScore(const PerformanceMetricsWindow& window) {
    if (window.epochs.empty()) return 1.0;

    double total_score = 0.0;
    for (const auto& epoch : window.epochs) {
        double score = 1.0;

        // Penalize long proof generation time
        if (epoch.proof_generation_time > 5000.0) {
            score -= 0.2;
        } else if (epoch.proof_generation_time > 2000.0) {
            score -= 0.1;
        }

        // Reward good utilization, penalize poor utilization
        if (epoch.prover_utilization < 0.3) {
            score -= 0.1;
        } else if (epoch.prover_utilization > 0.9) {
            score -= 0.2; // over utilized, performance score drops slightly to trigger scaling
        }

        // Penalize low backlog clearance
        if (epoch.backlog_clearance_rate < 1.0) {
            score -= (1.0 - epoch.backlog_clearance_rate) * 0.5;
        }

        if (score < 0.0) score = 0.0;
        total_score += score;
    }

    return std::clamp(total_score / static_cast<double>(window.epochs.size()), 0.0, 1.0);
}

double IslaModeEngine::computeEconomicScore(const EconomicMetricsWindow& window) {
    if (window.epochs.empty()) return 1.0;

    double total_score = 0.0;
    for (const auto& epoch : window.epochs) {
        double score = 1.0;

        // Penalize high fees
        if (epoch.sat_per_vbyte_fee_band > 50.0) {
            score -= 0.3;
        } else if (epoch.sat_per_vbyte_fee_band > 20.0) {
            score -= 0.1;
        }

        // Penalize high anchor cost ratio
        if (epoch.anchor_cost_ratio > 0.5) {
            score -= 0.2;
        }

        // Penalize high L2 backlog pressure
        if (epoch.l2_backlog_pressure > 0.8) {
            score -= 0.2;
        }

        if (score < 0.0) score = 0.0;
        total_score += score;
    }

    return std::clamp(total_score / static_cast<double>(window.epochs.size()), 0.0, 1.0);
}

IslaTuningDecision IslaModeEngine::computeDecision(
    const EpochMetricsWindow& epoch_metrics,
    const PerformanceMetricsWindow& perf_metrics,
    const EconomicMetricsWindow& econ_metrics,
    const IslaTuningDecision& current_state
) const {
    double coherence = computeCoherenceScore(epoch_metrics);
    double performance = computePerformanceScore(perf_metrics);
    double economic = computeEconomicScore(econ_metrics);

    IslaTuningDecision decision = current_state;

    // Calculate aggregated metrics across the window
    double avg_dispute_rate = 0.0;
    if (!epoch_metrics.epochs.empty()) {
        for (const auto& e : epoch_metrics.epochs) avg_dispute_rate += e.dispute_rate;
        avg_dispute_rate /= static_cast<double>(epoch_metrics.epochs.size());
    }

    double avg_proof_time = 0.0;
    double avg_utilization = 0.0;
    if (!perf_metrics.epochs.empty()) {
        for (const auto& e : perf_metrics.epochs) {
            avg_proof_time += e.proof_generation_time;
            avg_utilization += e.prover_utilization;
        }
        avg_proof_time /= static_cast<double>(perf_metrics.epochs.size());
        avg_utilization /= static_cast<double>(perf_metrics.epochs.size());
    }

    double avg_fees = 0.0;
    if (!econ_metrics.epochs.empty()) {
        for (const auto& e : econ_metrics.epochs) avg_fees += e.sat_per_vbyte_fee_band;
        avg_fees /= static_cast<double>(econ_metrics.epochs.size());
    }
    double avg_anchor_time = 0.0;
    if (!epoch_metrics.epochs.empty()) {
        for (const auto& e : epoch_metrics.epochs) avg_anchor_time += e.anchor_confirmation_time;
        avg_anchor_time /= static_cast<double>(epoch_metrics.epochs.size());
    }

    bool has_epoch_data = !epoch_metrics.epochs.empty();
    bool has_perf_data = !perf_metrics.epochs.empty();
    bool has_econ_data = !econ_metrics.epochs.empty();

    // --- Batch Size Rules ---
    // If coherence > 0.8 and performance > 0.7 → increase batch size (bounded)
    // If coherence < 0.5 → decrease batch size
    // If disputes rise → clamp batch size to minimum safe value
    if (has_epoch_data && avg_dispute_rate > 0.1) {
        decision.new_batch_size = MIN_BATCH_SIZE;
    } else if (has_epoch_data && coherence < 0.5) {
        // Decrease rule: -20% (rounded down)
        uint32_t decrease = (decision.new_batch_size * 20) / 100;
        if (decrease == 0) decrease = 1;

        if (decision.new_batch_size > decrease) {
            decision.new_batch_size -= decrease;
        } else {
            decision.new_batch_size = MIN_BATCH_SIZE;
        }
    } else if (has_epoch_data && has_perf_data && coherence > 0.8 && performance > 0.7) {
        // Increase rule: +10% (rounded up)
        uint32_t increase = (decision.new_batch_size * 10 + 99) / 100; // rough ceil
        if (increase == 0) increase = 1;
        decision.new_batch_size += increase;
    }

    // Clamp batch size to limits
    decision.new_batch_size = std::clamp(decision.new_batch_size, MIN_BATCH_SIZE, MAX_BATCH_SIZE);

    // --- Proof Interval Rules ---
    // If fees low & anchors early → shorten interval
    // If fees high or anchors delayed → lengthen interval
    if (has_econ_data && has_epoch_data && avg_fees < 10.0 && avg_anchor_time < 500.0) {
        // Shorten rule: -10%
        uint32_t decrease = (decision.new_proof_interval_ms * 10) / 100;
        if (decrease == 0) decrease = 1;

        if (decision.new_proof_interval_ms > decrease) {
            decision.new_proof_interval_ms -= decrease;
        } else {
            decision.new_proof_interval_ms = MIN_PROOF_INTERVAL_MS;
        }
    } else if ((has_econ_data && avg_fees > 30.0) || (has_epoch_data && avg_anchor_time > 2000.0)) {
        // Lengthen rule: +15%
        uint32_t increase = (decision.new_proof_interval_ms * 15) / 100;
        if (increase == 0) increase = 1;
        decision.new_proof_interval_ms += increase;
    }

    // Clamp proof interval to limits
    decision.new_proof_interval_ms = std::clamp(decision.new_proof_interval_ms, MIN_PROOF_INTERVAL_MS, MAX_PROOF_INTERVAL_MS);

    // --- Worker Allocation Rules ---
    // If proof times rising & utilization low → increase workers
    // If utilization high & coherence stable → maintain workers
    if (has_perf_data && avg_proof_time > 2000.0 && avg_utilization < 0.5) {
        decision.new_worker_allocation += 1;
    } else if (has_perf_data && avg_utilization > 0.9 && coherence > 0.8) {
        // maintain workers (no-op)
    } else if (has_perf_data && avg_utilization < 0.2) {
        // decrease rule just for safety if completely unutilized
        if (decision.new_worker_allocation > 1) {
            decision.new_worker_allocation -= 1;
        }
    }

    decision.new_worker_allocation = std::clamp(decision.new_worker_allocation, MIN_WORKER_ALLOCATION, MAX_WORKER_ALLOCATION);

    // --- Anchor Cadence Rules ---
    // If economic score low → anchor less frequently
    // If coherence high → anchor at normal cadence
    // If disputes rising → anchor more frequently for tighter continuity
    if (avg_dispute_rate > 0.1 || coherence < 0.5) {
        decision.new_anchor_cadence = AnchorCadence::TIGHT;
    } else if (economic < 0.5) {
        decision.new_anchor_cadence = AnchorCadence::RELAXED;
    } else if (coherence > 0.8) {
        decision.new_anchor_cadence = AnchorCadence::NORMAL;
    }

    return decision;
}

} // namespace ailee::l6::isla
