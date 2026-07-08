#include "l6/AnchorVerifier.h"
#include <cstring>

namespace ailee {
namespace l6 {

bool AnchorVerifier::verify(const AnchorRecord& rec, const AnchorState& current) const {
    if (rec.replay_height != current.replay_height) {
        return false;
    }
    if (rec.coherence_score != current.coherence_score) {
        return false;
    }
    if (std::memcmp(rec.state_root.data(), current.state_root.data(), 32) != 0) {
        return false;
    }
    return true;
}

} // namespace l6
} // namespace ailee
