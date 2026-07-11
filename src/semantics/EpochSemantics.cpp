#include "l6/IslaRuntimeOrchestrator.h"
#include "semantics/EpochSemantics.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace ailee::semantics {

uint64_t EpochSemantics::compute_epoch_id(const l6::ClockSnapshot& snapshot, uint32_t blocks_per_epoch) {
    if (blocks_per_epoch == 0) return 0;
    return snapshot.height / blocks_per_epoch;
}

bool EpochSemantics::validate_epoch_bundle(const l6::EpochIntegrationBundle& bundle, const l6::IReplayBuffer& replay_buffer, uint32_t blocks_per_epoch) {
    if (bundle.constraints == nullptr || bundle.constraints->num_constraints == 0) {
        return false;
    }

    if (bundle.transcript == nullptr || bundle.transcript->num_rounds == 0) {
        return false;
    }

    // Hash transcript bytes
    std::vector<uint8_t> transcript_bytes = bundle.transcript->to_bytes();
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(transcript_bytes.data(), transcript_bytes.size(), hash.data());

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    std::string canonical_hash = ss.str();

    if (bundle.state_root_hash != canonical_hash) {
        return false;
    }

    // Check height against replay buffer
    auto history = replay_buffer.get_epoch_history();
    uint64_t previous_height = 0;
    if (!history.empty()) {
        previous_height = history.back().clock_snapshot.height;
    }

    if (bundle.clock_snapshot.height < previous_height) {
        return false;
    }

    // Check epoch id
    if (bundle.epoch_id != compute_epoch_id(bundle.clock_snapshot, blocks_per_epoch)) {
        return false;
    }

    return true;
}

} // namespace ailee::semantics
