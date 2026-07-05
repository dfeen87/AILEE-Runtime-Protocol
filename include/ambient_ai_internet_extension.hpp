#include <cstdint>
#include <vector>
#include <string>
#ifndef AMBIENT_AI_INTERNET_EXTENSION_HPP
#define AMBIENT_AI_INTERNET_EXTENSION_HPP

#include <vector>
#include <array>

namespace ailee {
namespace ambient {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientEvent {
    Hash256 eventId;
    std::string deviceId;
    uint64_t logicalTimestamp;
    std::vector<uint8_t> payload; // Bounded to 1024 bytes per protocol rules

    // Cryptographically binds the payload to the device
    std::vector<uint8_t> deviceSignature;

    bool verifySignature() const;
    Hash256 hash() const;
};

class AmbientEventAggregator {
public:
    // Pushes an event into the current epoch's memory pool
    bool submitEvent(const AmbientEvent& event);

    // Deterministically sorts and hashes all events to produce the ambientEventRoot
    // Conflict resolution: If duplicate eventIds exist, the one with the lowest
    // lexicographical deviceSignature is retained.
    Hash256 finalizeEpochRoot() const;
};

} // namespace ambient
} // namespace ailee

#endif // AMBIENT_AI_INTERNET_EXTENSION_HPP
