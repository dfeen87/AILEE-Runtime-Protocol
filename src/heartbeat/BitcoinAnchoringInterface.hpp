#ifndef AILEE_HEARTBEAT_BITCOIN_ANCHORING_INTERFACE_HPP
#define AILEE_HEARTBEAT_BITCOIN_ANCHORING_INTERFACE_HPP

#include <cstdint>
#include <string>

namespace ailee {
namespace heartbeat {

struct AnchorResult {
    std::string commitment_hash;
    std::string anchor_metadata;
};

class IBitcoinAnchor {
public:
    virtual ~IBitcoinAnchor() = default;

    virtual AnchorResult anchor_state_root(
        uint64_t epoch,
        const std::string& state_root
    ) = 0;
};

class DeterministicBitcoinAnchor : public IBitcoinAnchor {
public:
    AnchorResult anchor_state_root(
        uint64_t epoch,
        const std::string& state_root
    ) override;
};

} // namespace heartbeat
} // namespace ailee

#endif // AILEE_HEARTBEAT_BITCOIN_ANCHORING_INTERFACE_HPP
