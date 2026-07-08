#include "l4/DeterministicScheduler.h"
#include "l4/RecoveryCoordinator.h"
#include "l4/DeterministicTransport.h"
#include "l3/GossipLayer.h"
#include "l3/PeerSync.h"
#include "NodeIdentity.h"
#include "l4/ReplayEngine.h"
#include "l4/ReplayTick.h"
#include <cstring>
#include <iostream>

namespace ailee {
namespace l4 {

DeterministicScheduler::DeterministicScheduler() : replay_engine(std::make_unique<ReplayEngine>()) {
    std::memset(&state, 0, sizeof(state));
    std::memset(&current_epoch, 0, sizeof(current_epoch));
    std::memset(&current_anchor, 0, sizeof(current_anchor));
    std::memset(&previous_replay_state, 0, sizeof(previous_replay_state));
}

DeterministicScheduler::~DeterministicScheduler() = default;

void DeterministicScheduler::run_tick(
    ClusterView& view,
    const std::vector<std::pair<size_t, size_t>>& gossip_schedule,
    std::vector<l2::DeterministicEngine>& engines
) {
    auto events = mainnet_sync.drain_sync_events();
    auto clock = mainnet_sync.get_clock();

    l1_sync::ReplayInput input{events, clock};
    ReplayTick tick = replay_engine->step(previous_replay_state, input);

    previous_replay_state.current_height = tick.height;
    latest_replay_tick = std::make_unique<ReplayTick>(tick);

    view.clock = tick.clock;
    view.replay_events = tick.replay_events;

    SchedulerPhase phase = static_cast<SchedulerPhase>(state.tick_count % 9);

    switch (phase) {
        case SchedulerPhase::ENGINE_STEP: {
            for (size_t i = 0; i < view.nodes.size(); ++i) {
                ClusterNodeState& node = view.nodes[i];

                reflection::ReflectionSnapshot dummy_reflection = {};
                l1::SettlementIngestion dummy_settlement = {};

                const auto& clock = mainnet_sync.get_clock();
                dummy_settlement.latest_anchor.bitcoinHeight = clock.height;
                dummy_settlement.latest_anchor.broadcastTime = static_cast<uint64_t>(clock.consensus_time);
                mesh::MeshCoherenceResult dummy_coherence = {};

                identity::NodeId node_id;
                std::memset(&node_id, 0, sizeof(node_id));
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
                node.last_envelope.context = result.new_state.context;
                std::memset(node.last_envelope.signature_or_commitment, 0, sizeof(node.last_envelope.signature_or_commitment));
                node.step_counter++;

                l3::GossipMessage self_msg = l3::build_gossip_message(
                    node.last_envelope,
                    dummy_coherence,
                    node.step_counter
                );
                node.last_gossip_summary = self_msg.summary;
            }
            break;
        }

        case SchedulerPhase::GOSSIP_PEER_SYNC: {
            for (const auto& pair : gossip_schedule) {
                size_t sender_idx = pair.first;
                size_t receiver_idx = pair.second;

                if (sender_idx >= view.nodes.size() || receiver_idx >= view.nodes.size()) {
                    continue;
                }

                ClusterNodeState& sender = view.nodes[sender_idx];
                ClusterNodeState& receiver = view.nodes[receiver_idx];

                mesh::MeshCoherenceResult dummy_coherence = {};
                l3::GossipMessage gossip_msg = l3::build_gossip_message(
                    sender.last_envelope,
                    dummy_coherence,
                    sender.step_counter
                );

                l3::GossipEnvelope normalized_gossip = l3::receive_gossip_message(gossip_msg);

                l3::PeerSyncState sync_state = l3::compute_peer_sync(
                    receiver.last_envelope,
                    normalized_gossip
                );

                receiver.peer_sync_states.push_back(sync_state);
            }
            break;
        }

        case SchedulerPhase::RECOVERY: {
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
            break;
        }

        case SchedulerPhase::MESH_EPOCH_BUILD: {
            current_epoch = build_mesh_epoch(view);
            state.epoch_height = current_epoch.epoch_height;
            // Also update last_mesh_state_root_hash by casting the first 8 bytes of mesh_state_root
            uint64_t root_hash_prefix = 0;
            std::memcpy(&root_hash_prefix, current_epoch.mesh_state_root, sizeof(uint64_t));
            state.last_mesh_state_root_hash = root_hash_prefix;
            break;
        }

        case SchedulerPhase::MESH_ANCHOR_BUILD: {
            current_anchor = build_mesh_anchor(current_epoch, view);
            state.last_anchor_id = current_anchor.anchor_id;

            view.mesh_envelopes.clear();
            view.mesh_envelopes.reserve(view.nodes.size());
            for (const auto& node : view.nodes) {
                MeshPropagationEnvelope env = build_mesh_propagation_envelope(node.node_id_hash, current_anchor);
                view.mesh_envelopes.push_back(env);
            }
            break;
        }

        case SchedulerPhase::STATE_ROOT_ANNOUNCEMENTS: {
            current_announcements = build_state_root_announcements(view);
            break;
        }

        case SchedulerPhase::STATE_ROOT_VALIDATION: {
            auto validation_results = validate_state_roots(view, current_anchor, current_announcements);

            for (auto& node : view.nodes) {
                StateRootStatus new_status = StateRootStatus::UNKNOWN;
                for (const auto& res : validation_results) {
                    if (res.node_id_hash == node.node_id_hash) {
                        if (res.accepted) {
                            new_status = StateRootStatus::CONSISTENT;
                        } else if (res.rejected) {
                            new_status = StateRootStatus::INCONSISTENT;
                        }
                        break;
                    }
                }
                node.state_root_status = new_status;
            }
            break;
        }

        case SchedulerPhase::TRANSPORT_DELIVERY: {
            for (const auto& node : view.nodes) {
                // ENVELOPE
                for (const auto& target : view.nodes) {
                    TransportMessage msg_env = {};
                    msg_env.source_node_id_hash = node.node_id_hash;
                    msg_env.dest_node_id_hash = target.node_id_hash;
                    msg_env.epoch_height = node.last_envelope.context.l1_height;
                    msg_env.message_type = 0; // ENVELOPE
                    pack_envelope_payload(msg_env.payload, node.last_envelope.context.l1_height, node.last_envelope.context.state_root_hash);
                    enqueue_transport_message(view.transport_queue, msg_env);
                }

                // STATE_ROOT_ANNOUNCEMENT
                for (const auto& ann : current_announcements) {
                    if (ann.source_node_id_hash == node.node_id_hash) {
                        for (const auto& target : view.nodes) {
                            TransportMessage msg_ann = {};
                            msg_ann.source_node_id_hash = node.node_id_hash;
                            msg_ann.dest_node_id_hash = target.node_id_hash;
                            msg_ann.epoch_height = ann.epoch_height;
                            msg_ann.message_type = 1; // STATE_ROOT_ANNOUNCEMENT
                            pack_state_root_announcement_payload(msg_ann.payload, ann.epoch_height, ann.state_root);
                            enqueue_transport_message(view.transport_queue, msg_ann);
                        }
                    }
                }

                // MESH_ANCHOR
                for (const auto& target : view.nodes) {
                    TransportMessage msg_anch = {};
                    msg_anch.source_node_id_hash = node.node_id_hash;
                    msg_anch.dest_node_id_hash = target.node_id_hash;
                    msg_anch.epoch_height = current_anchor.epoch.epoch_height;
                    msg_anch.message_type = 2; // MESH_ANCHOR
                    pack_mesh_anchor_payload(msg_anch.payload, current_anchor.epoch.epoch_height, current_anchor.epoch.mesh_state_root);
                    enqueue_transport_message(view.transport_queue, msg_anch);
                }
            }

            // Drain and discard
            for (const auto& node : view.nodes) {
                std::vector<TransportMessage> node_messages = drain_transport_messages_for_node(
                    view.transport_queue, node.node_id_hash);

                for (const auto& msg : node_messages) {
                    (void)msg; // discard
                }
            }
            break;
        }

        case SchedulerPhase::COHERENCE_UPDATE: {
            view.coherence_summary = compute_cluster_coherence(view);
            state.cluster_stable = (view.coherence_summary.global_coherence_score == 100);
            view.total_steps++;
            record_telemetry_sample(view, state, telemetry);
            last_snapshot = dashboard_builder.build_snapshot(telemetry);
            break;
        }
    }

    state.tick_count++;
}

} // namespace l4
} // namespace ailee
