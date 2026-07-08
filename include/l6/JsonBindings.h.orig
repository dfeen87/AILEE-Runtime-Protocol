#pragma once

#include <string>
#include "l6/ExternalSchema.h"

namespace ailee {
namespace l6 {

class JsonBindings {
public:
    static std::string to_json(const ExternalEnvelope& env);
    static std::string to_json(const ExternalClusterView& view);
    static std::string to_json(const ExternalReplayTick& tick);

    static ExternalEnvelope from_json_envelope(const std::string& json);
    static ExternalClusterView from_json_view(const std::string& json);
    static ExternalReplayTick from_json_tick(const std::string& json);
};

} // namespace l6
} // namespace ailee
