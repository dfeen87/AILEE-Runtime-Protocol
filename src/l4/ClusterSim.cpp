#include "l4/ClusterSim.h"
#include "l4/RecoveryCoordinator.h"
#include "l4/MeshAnchor.h"
#include "l2/DeterministicEngine.h"
#include "l3/GossipLayer.h"
#include "l3/PeerSync.h"
#include "NodeIdentity.h"
#include <cstring>
#include <iostream>

namespace ailee {
namespace l4 {

ClusterView run_cluster_simulation(
    const std::vector<ClusterNodeState>& initial_nodes,
    const std::vector<std::pair<size_t, size_t>>& gossip_schedule,
    uint64_t max_steps
) {
    ClusterView view = {};
    view.nodes = initial_nodes;
    view.total_nodes = initial_nodes.size();
    view.total_steps = 0;

    // We instantiate DeterministicEngines per node to hold their internal state
    std::vector<l2::DeterministicEngine> engines;
    engines.reserve(view.nodes.size());
    for (const auto& node : view.nodes) {
        engines.emplace_back(node.engine_state);
    }

    for (uint64_t step = 0; step < max_steps; ++step) {

        // 1. Deterministic Gossip Exchange
        for (const auto& pair : gossip_schedule) {
            size_t sender_idx = pair.first;
            size_t receiver_idx = pair.second;

            if (sender_idx >= view.nodes.size() || receiver_idx >= view.nodes.size()) {
                continue;
            }

            ClusterNodeState& sender = view.nodes[sender_idx];
            ClusterNodeState& receiver = view.nodes[receiver_idx];

            // Build gossip message (zero-initialized mesh coherence for simulation)
            mesh::MeshCoherenceResult dummy_coherence = {};
            l3::GossipMessage gossip_msg = l3::build_gossip_message(
                sender.last_envelope,
                dummy_coherence,
                sender.step_counter
            );

            // Receiver normalizes it
            l3::GossipEnvelope normalized_gossip = l3::receive_gossip_message(gossip_msg);

            // Receiver computes peer sync
            l3::PeerSyncState sync_state = l3::compute_peer_sync(
                receiver.last_envelope,
                normalized_gossip
            );

            // Receiver updates its peer_sync_states
            receiver.peer_sync_states.push_back(sync_state);
        }

        // 1.5. Deterministic Recovery
        std::vector<RecoveryRequest> requests = build_recovery_requests(view);
        std::vector<RecoveryResponse> responses = match_recovery_responses(view, requests);

        for (const auto& response : responses) {
            for (size_t i = 0; i < view.nodes.size(); ++i) {
                if (view.nodes[i].node_id_hash == response.requester_node_id_hash) {
                    apply_recovery(view.nodes[i], response);
                    engines[i].set_state(view.nodes[i].engine_state);
                    break;
                }
            }
        }

        // 2. Deterministic Engine Progression & 3. Update Node State
        for (size_t i = 0; i < view.nodes.size(); ++i) {
            ClusterNodeState& node = view.nodes[i];

            reflection::ReflectionSnapshot dummy_reflection = {};
            l1::SettlementIngestion dummy_settlement = {};
            mesh::MeshCoherenceResult dummy_coherence = {};

            identity::NodeId node_id;
            std::memset(&node_id, 0, sizeof(node_id));
            // Let's populate node_id id with node_id_hash
            std::memcpy(node_id.id, &node.node_id_hash, sizeof(node.node_id_hash));

            uint32_t protocol_version = 1;

            l2::EngineStepResult result = engines[i].step(
                dummy_reflection,
                dummy_settlement,
                dummy_coherence,
                node_id,
                protocol_version
            );

            node.engine_state = result.new_state;

            // Update last_envelope from new state context
            node.last_envelope.context = result.new_state.context;
            std::memset(node.last_envelope.signature_or_commitment, 0, sizeof(node.last_envelope.signature_or_commitment));

            node.step_counter++;

            // Re-build gossip summary for this node based on new state
            l3::GossipMessage self_msg = l3::build_gossip_message(
                node.last_envelope,
                dummy_coherence,
                node.step_counter
            );
            node.last_gossip_summary = self_msg.summary;
        }

        // 4. Mesh Anchoring (at the end of the step)
        MeshEpoch epoch = build_mesh_epoch(view);
        MeshAnchor anchor = build_mesh_anchor(epoch, view);

        view.mesh_envelopes.clear(); // Ensure it only holds envelopes for current step, or we can append. The prompt asks to "optionally produce mesh-anchored output ... Optionally store these envelopes in a new field on ClusterView". Since there's one per node per step, appending makes sense if we want history, but usually view is the current state. The instructions say "For each node, build a MeshPropagationEnvelope ... Optionally store these envelopes in a new field on ClusterView: e.g., std::vector<MeshPropagationEnvelope> mesh_envelopes;". Let's clear and store the latest.
        view.mesh_envelopes.reserve(view.nodes.size());
        for (const auto& node : view.nodes) {
            MeshPropagationEnvelope env = build_mesh_propagation_envelope(node.node_id_hash, anchor);
            view.mesh_envelopes.push_back(env);
        }

        view.total_steps++;
        }

    return view;
}

ClusterCoherenceSummary compute_cluster_coherence(const ClusterView& view) {
    ClusterCoherenceSummary summary = {};
    std::memset(&summary, 0, sizeof(summary));

    if (view.total_nodes == 0) {
        return summary;
    }

    std::vector<RecoveryRequest> pending_reqs = build_recovery_requests(view);
    std::vector<RecoveryResponse> matched_resps = match_recovery_responses(view, pending_reqs);

    summary.unrecoverable_nodes_count = pending_reqs.size() - matched_resps.size();

    for (const auto& node : view.nodes) {
        if (!node.peer_sync_states.empty()) {
            const l3::PeerSyncState& latest_sync = node.peer_sync_states.back();

            switch (latest_sync.sync_status) {
                case l3::SyncStatus::IN_SYNC:
                    summary.in_sync_count++;
                    break;
                case l3::SyncStatus::AHEAD:
                    summary.ahead_count++;
                    break;
                case l3::SyncStatus::BEHIND:
                    summary.behind_count++;
                    break;
                case l3::SyncStatus::NEEDS_RECOVERY:
                    summary.needs_recovery_count++;
                    break;
                case l3::SyncStatus::STALE:
                    summary.stale_count++;
                    break;
            }

            bool was_bad = false;
            for (const auto& st : node.peer_sync_states) {
                if (st.sync_status == l3::SyncStatus::NEEDS_RECOVERY || st.sync_status == l3::SyncStatus::BEHIND) {
                    was_bad = true;
                }
                if (was_bad && st.sync_status == l3::SyncStatus::IN_SYNC) {
                    summary.recovered_nodes_count++;
                    break;
                }
            }
        } else {
            // Assume stale if no sync state
            summary.stale_count++;
        }
    }

    summary.global_coherence_score = (summary.in_sync_count * 100) / view.total_nodes;

    return summary;
}

} // namespace l4
} // namespace ailee
