#include "ZKWiring.h"
#include "../clock/ConsensusClock.h"

namespace ailee {
namespace zk {

ReplayProofArtifact run_full_v18_pipeline(const anchor::AnchorRecord& record) {
    // 1. Compute anchor_root = record.anchor_root()
    std::array<uint8_t, 32> anchor_root = record.anchor_root();

    // 2. Instantiate ConsensusClock clock
    clock::ConsensusClock clock;

    // 3. Call clock.update(record.epoch_id, anchor_root)
    // 4. Receive ClockPacket packet
    clock::ClockPacket packet = clock.update(record.epoch_id, anchor_root);

    // 5. Instantiate ZKReplayCircuit zk
    ZKReplayCircuit zk;

    // 6. Return zk.generate(packet)
    return zk.generate(packet);
}

} // namespace zk
} // namespace ailee
