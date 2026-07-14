#include "l3/NetworkBinding.h"
#include <cstring>
#include <algorithm>

namespace ailee {
namespace l3 {

// ============================================================================
// MAINNET PEER ROUTING SUPPORT (NEW)
// ============================================================================
// We maintain a minimal routing table mapping peerId → address.
// GossipLayer + PeerSync populate peer IDs; BroadcastEngine uses addresses.
// ============================================================================
static std::unordered_map<std::string, std::string> g_peer_routes;

// Register or update a peer route
void NetworkBinding::registerPeerAddress(const std::string& peerId,
                                         const std::string& address)
{
    g_peer_routes[peerId] = address;
}

// Lookup a peer address (returns empty string if unknown)
std::string NetworkBinding::getPeerAddress(const std::string& peerId) const
{
    auto it = g_peer_routes.find(peerId);
    if (it == g_peer_routes.end()) return "";
    return it->second;
}

// Broadcast to a specific mainnet peer
void NetworkBinding::broadcastTo(const std::string& address,
                                 const std::string& data)
{
    // In a real system, this would be a QUIC/libp2p/TCP send.
    // For now, this is a stub that preserves architecture.
    // TODO: integrate with your actual transport layer.
    (void)address;
    (void)data;
}

// ============================================================================
// EXISTING LOGIC (UNCHANGED)
// ============================================================================
// These functions bind network snapshots into deterministic protocol structures.
// ============================================================================

reflection::ReflectionSnapshot bind_network_block(const NetworkBlockSnapshot& net_block) {
    reflection::ReflectionSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    snap.height.height = net_block.height;

    // Copy the block hash directly to the anchor hash since the block itself
    // is the anchor in this abstraction.
    std::memcpy(snap.anchor.anchor_hash, net_block.block_hash, 32);
    snap.anchor.block_height = net_block.height;

    // No reorg info available in a single snapshot.
    return snap;
}

l1::SettlementIngestion bind_network_mempool(const NetworkMempoolSnapshot& net_mempool,
                                             const NetworkBlockSnapshot& net_block)
{
    l1::SettlementIngestion ingest;
    std::memset(&ingest, 0, sizeof(ingest));

    // Bind mempool snapshot size as anchor processed counts.
    std::memcpy(ingest.latest_anchor.anchorHash, net_block.block_hash, 32);
    ingest.latest_anchor.bitcoinHeight = net_block.height;
    ingest.latest_anchor.status = 1; // CONFIRMED

    ingest.total_anchors_processed = net_mempool.transaction_count;
    ingest.latest_confirmations = 1;

    return ingest;
}

mesh::MeshCoherenceResult bind_network_peer(const NetworkPeerSnapshot& net_peer) {
    mesh::MeshCoherenceResult result;
    std::memset(&result, 0, sizeof(result));

    // Map peer metrics to deterministic coherence score.
    result.score = static_cast<uint32_t>(
        std::min<uint64_t>(net_peer.active_connections, 4)
    );

    if (result.score > 0) result.l1_height_match = true;
    if (result.score > 1) result.anchor_match = true;
    if (result.score > 2) result.epoch_match = true;
    if (result.score > 3) result.state_root_match = true;

    return result;
}

} // namespace l3
} // namespace ailee
