#pragma once

#include <array>
#include <cstdint>
#include "l4/ReplayBuffer.h"
#include "l4/ClusterFederationView.h"

namespace ailee {
namespace l6 {

struct AnchorState {
    std::array<uint8_t, 32> state_root;
    uint64_t replay_height;
    uint64_t coherence_score;
};

class StateRootBuilder {
public:
    std::array<uint8_t, 32> build_state_root(
        const l4::ReplayBuffer& replay,
        const l4::ClusterFederationView& federation
    );
};

} // namespace l6
} // namespace ailee
