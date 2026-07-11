#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "l4/StateRootPropagation.h"
#include <openssl/sha.h>
#include <algorithm>
#include <cstring>

namespace ailee {
namespace l4 {

namespace {

// Helper to serialize uint64_t into a byte array in little-endian format
void serialize_uint64_le(uint64_t val, uint8_t out[8]) {
    out[0] = static_cast<uint8_t>(val & 0xFF);
    out[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    out[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    out[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    out[4] = static_cast<uint8_t>((val >> 32) & 0xFF);
    out[5] = static_cast<uint8_t>((val >> 40) & 0xFF);
    out[6] = static_cast<uint8_t>((val >> 48) & 0xFF);
    out[7] = static_cast<uint8_t>((val >> 56) & 0xFF);
}

} // anonymous namespace

std::vector<StateRootAnnouncement> build_state_root_announcements(const ClusterView& view) {
    std::vector<StateRootAnnouncement> announcements;
    announcements.reserve(view.nodes.size());

    for (const auto& node : view.nodes) {
        StateRootAnnouncement ann = {};
        std::memset(&ann, 0, sizeof(ann));

        ann.source_node_id_hash = node.node_id_hash;
        ann.epoch_height = node.last_envelope.context.l1_height;
        std::memcpy(ann.state_root, node.last_envelope.context.state_root_hash, 32);

        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        uint8_t tmp[8];
        serialize_uint64_le(ann.source_node_id_hash, tmp);
        SHA256_Update(&ctx, tmp, 8);

        serialize_uint64_le(ann.epoch_height, tmp);
        SHA256_Update(&ctx, tmp, 8);

        SHA256_Update(&ctx, ann.state_root, 32);

        SHA256_Final(ann.announcement_hash, &ctx);

        announcements.push_back(ann);
    }

    // Sort by source_node_id_hash ascending
    std::sort(announcements.begin(), announcements.end(), [](const StateRootAnnouncement& a, const StateRootAnnouncement& b) {
        return a.source_node_id_hash < b.source_node_id_hash;
    });

    return announcements;
}

std::vector<StateRootValidationResult> validate_state_roots(
    const ClusterView& view,
    const MeshAnchor& anchor,
    const std::vector<StateRootAnnouncement>& announcements) {

    std::vector<StateRootValidationResult> results;
    results.reserve(view.nodes.size());

    for (const auto& node : view.nodes) {
        StateRootValidationResult res = {};
        std::memset(&res, 0, sizeof(res));
        res.node_id_hash = node.node_id_hash;
        res.epoch_height = anchor.epoch.epoch_height;

        const StateRootAnnouncement* my_ann = nullptr;
        for (const auto& ann : announcements) {
            if (ann.source_node_id_hash == node.node_id_hash) {
                my_ann = &ann;
                break;
            }
        }

        if (my_ann) {
            if (my_ann->epoch_height == anchor.epoch.epoch_height &&
                std::memcmp(my_ann->state_root, anchor.epoch.mesh_state_root, 32) == 0) {
                res.accepted = true;
                res.rejected = false;
                res.reason_code = 0; // OK
            } else {
                res.accepted = false;
                res.rejected = true;
                if (my_ann->epoch_height < anchor.epoch.epoch_height) {
                    res.reason_code = 2; // STALE_EPOCH
                } else {
                    res.reason_code = 1; // MISMATCH_ANCHOR
                }
            }
        } else {
            // Find if there's any announcement at all with unknown source
            bool has_unknown = false;
            for (const auto& ann : announcements) {
                bool found = false;
                for (const auto& vnode : view.nodes) {
                    if (vnode.node_id_hash == ann.source_node_id_hash) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    has_unknown = true;
                    break;
                }
            }

            res.accepted = false;
            res.rejected = true;
            // Either way, if there's no matching announcement, we treat it as UNKNOWN_SOURCE
            res.reason_code = 3; // UNKNOWN_SOURCE
        }

        results.push_back(res);
    }

    // Sort by node_id_hash ascending
    std::sort(results.begin(), results.end(), [](const StateRootValidationResult& a, const StateRootValidationResult& b) {
        return a.node_id_hash < b.node_id_hash;
    });

    return results;
}

} // namespace l4
} // namespace ailee
