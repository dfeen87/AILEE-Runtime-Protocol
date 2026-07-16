#pragma once

#include <string>
#include "l6/ExternalSchema.h"

namespace ailee {
namespace l6 {

class JsonBindings {
public:
    static std::string to_json(const ExternalEnvelope& env);
    static std::string to_json(const ExternalBitcoinClockState& clock);
    static std::string to_json(const ExternalReplayEvent& event);
    static std::string to_json(const ExternalClusterView& view);
    static std::string to_json(const ExternalReplayTick& tick);

    // V35 serialisation helpers
    static std::string to_json(const ExternalActiveSession& session);
    static std::string to_json(const ExternalBroadcastQueue& queue);
    static std::string to_json(const ExternalStateSnapshot& snapshot);

    static ExternalEnvelope from_json_envelope(const std::string& json);
    static ExternalBitcoinClockState from_json_clock(const std::string& json);
    static ExternalReplayEvent from_json_replay_event(const std::string& json);
    static ExternalClusterView from_json_view(const std::string& json);
    static ExternalReplayTick from_json_tick(const std::string& json);

    // V35 deserialisation helpers
    static ExternalActiveSession from_json_active_session(const std::string& json);
    static ExternalBroadcastQueue from_json_broadcast_queue(const std::string& json);
    static ExternalStateSnapshot from_json_state_snapshot(const std::string& json);
};

} // namespace l6
} // namespace ailee
