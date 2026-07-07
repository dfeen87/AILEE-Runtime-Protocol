#pragma once

#include <cstdint>
#include "ReflectionLayer.h"
#include "SettlementIngestion.h"
#include "MeshCoherence.h"

namespace ailee {
namespace l2 {

struct alignas(64) StateRoot {
    uint8_t root_hash[32];
    uint64_t l1_height_basis;
    uint32_t ingestion_count;
    uint32_t coherence_score;
    uint8_t padding[16]; // 32 + 8 + 4 + 4 + 16 = 64
};
static_assert(sizeof(StateRoot) == 64, "StateRoot must be 64 bytes");

struct alignas(64) StateRootPipelineResult {
    StateRoot generated_root; // 64
    uint8_t reflection_hash_used[32]; // 32
    uint32_t anchors_ingested; // 4
    bool coherence_passed; // 1
    uint8_t padding[27]; // 64 + 32 + 4 + 1 + 27 = 128
};
static_assert(sizeof(StateRootPipelineResult) == 128, "StateRootPipelineResult must be 128 bytes");

StateRoot compute_state_root(
    const reflection::ReflectionSnapshot& reflection,
    const l1::SettlementIngestion& ingestion,
    const mesh::MeshCoherenceResult& coherence
);

} // namespace l2
} // namespace ailee
