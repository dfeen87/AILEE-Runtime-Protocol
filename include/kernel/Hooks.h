#pragma once

#include <string>
#include <cstdint>

namespace ailee {

struct Route;

namespace l6 {
struct ExternalStateSnapshot;
struct ExternalReplayTick;
}

namespace kernel {

struct StateSnapshotContext {
    std::string initiator = "system";
};

struct ReplayTickContext {
    uint64_t tick_index = 0;
    std::string status = "success";
};

class Hooks {
public:
    static void onRouteRegistered(const Route& route);
    static void onStateSnapshotRequested(const StateSnapshotContext& ctx, l6::ExternalStateSnapshot& snapshot);
    static void onReplayTick(const ReplayTickContext& ctx, const l6::ExternalReplayTick& tick);
};

} // namespace kernel
} // namespace ailee
