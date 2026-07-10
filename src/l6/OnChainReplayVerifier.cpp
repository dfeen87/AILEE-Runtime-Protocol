#include "l6/OnChainReplayVerifier.h"

namespace ailee {
namespace l6 {

ReplayVerdict verify_epoch_against_anchor(
    uint64_t epoch_id,
    const std::string& local_epoch_hash,
    const std::optional<std::string>& anchor_hash_from_l1
) {
    ReplayVerdict verdict;
    verdict.epoch_id = epoch_id;
    verdict.local_hash = local_epoch_hash;

    if (!anchor_hash_from_l1.has_value()) {
        verdict.status = ReplayStatus::MISSING_ANCHOR;
        verdict.anchor_hash = "";
    } else if (local_epoch_hash != anchor_hash_from_l1.value()) {
        verdict.status = ReplayStatus::MISMATCH;
        verdict.anchor_hash = anchor_hash_from_l1.value();
    } else {
        verdict.status = ReplayStatus::OK;
        verdict.anchor_hash = anchor_hash_from_l1.value();
    }

    return verdict;
}

} // namespace l6
} // namespace ailee
