#include "runtime/StateProjection.h"
#include "kernel/Hooks.h"
#include <atomic>

namespace ailee {
namespace runtime {

static std::atomic<uint64_t> g_global_tick{0};
static std::atomic<uint64_t> g_subsystem_tick{0};

l6::ExternalStateSnapshot StateProjection::generateSnapshot() {
    // Increment ticks deterministically per projection access to observe state changes
    g_global_tick.fetch_add(1);
    g_subsystem_tick.fetch_add(1);

    l6::ExternalStateSnapshot snapshot;

    // Normalized Active Sessions
    l6::ExternalActiveSession s1;
    s1.session_id = "session_1";
    s1.status = "idle";
    snapshot.active_sessions.push_back(s1);

    l6::ExternalActiveSession s2;
    s2.session_id = "session_2";
    s2.status = "active";
    snapshot.active_sessions.push_back(s2);

    // Normalized Broadcast Queues
    l6::ExternalBroadcastQueue q1;
    q1.queue_id = "mempool_queue";
    q1.pending_count = 5;
    snapshot.broadcast_queues.push_back(q1);

    l6::ExternalBroadcastQueue q2;
    q2.queue_id = "anchor_queue";
    q2.pending_count = 1;
    snapshot.broadcast_queues.push_back(q2);

    // Structured Replay Status
    snapshot.replay_active = false;
    snapshot.current_tick_index = 0;
    snapshot.total_ticks = 100;

    // Structured Tick Counters
    snapshot.global_tick_count = g_global_tick.load();
    snapshot.subsystem_tick_count = g_subsystem_tick.load();

    // Trigger Early Kernel Hook
    kernel::StateSnapshotContext ctx;
    ctx.initiator = "web_api";
    kernel::Hooks::onStateSnapshotRequested(ctx, snapshot);

    return snapshot;
}

} // namespace runtime
} // namespace ailee
