#include "l5/DeterministicCompressor.h"
#include <stdexcept>
#include <cstring>

namespace ailee {
namespace l5 {

namespace {

// Deterministic FNV-1a 32-bit hash
uint32_t fnv1a_32(const std::vector<uint8_t>& data) {
    uint32_t hash = 2166136261u;
    for (uint8_t byte : data) {
        hash ^= byte;
        hash *= 16777619u;
    }
    return hash;
}

} // anonymous namespace

DeterministicCompressor::DeterministicCompressor(const CompressionConfig& cfg)
    : config(cfg), has_previous(false)
{}

std::vector<uint8_t> DeterministicCompressor::compress_tick(
    const l4::ClusterView& /* view */,
    const l4::ReplayTick& tick
) {
    auto current_serialized = tick.serialize();

    // Step 1: delta encode
    std::vector<uint8_t> delta;
    if (config.enable_delta && has_previous) {
        delta = delta_encode(previous_serialized, current_serialized);
    } else {
        delta = current_serialized;
    }

    previous_serialized = current_serialized;
    has_previous = true;

    // Step 2: RLE encode
    auto rle = config.enable_rle
        ? rle_encode(delta)
        : delta;

    // Step 3: stable hash (optional)
    auto hashed = config.enable_stable_hash
        ? stable_hash(rle)
        : rle;

    // Step 4: enforce deterministic size cap
    if (hashed.size() > config.max_compressed_bytes) {
        throw std::runtime_error("Deterministic compression overflow");
    }

    return hashed;
}

l4::ReplayTick DeterministicCompressor::decompress_tick(
    const std::vector<uint8_t>& data
) {
    std::vector<uint8_t> rle_input = data;

    // Reverse stable hash if enabled
    if (config.enable_stable_hash) {
        if (data.size() < 4) {
            throw std::runtime_error("Decompression failed: Missing hash");
        }
        uint32_t expected_hash;
        std::memcpy(&expected_hash, data.data() + data.size() - 4, 4);

        rle_input.resize(data.size() - 4);

        uint32_t actual_hash = fnv1a_32(rle_input);
        if (actual_hash != expected_hash) {
            throw std::runtime_error("Decompression failed: Hash mismatch");
        }
    }

    auto rle_decoded = config.enable_rle
        ? rle_decode(rle_input)
        : rle_input;

    std::vector<uint8_t> delta_decoded;
    if (config.enable_delta && has_previous) { // Assuming decompression is stateful like compression
        delta_decoded = delta_decode(previous_serialized, rle_decoded);
    } else {
        delta_decoded = rle_decoded;
    }

    previous_serialized = delta_decoded;
    has_previous = true;

    return l4::ReplayTick::deserialize(delta_decoded);
}

std::vector<uint8_t> DeterministicCompressor::delta_encode(const std::vector<uint8_t>& previous, const std::vector<uint8_t>& current) {
    // Simple byte-level delta
    // [size of current (8 bytes)] + [delta bytes for min(prev, curr) length] + [append extra current bytes if curr > prev]

    std::vector<uint8_t> out;
    uint64_t curr_size = current.size();
    out.resize(sizeof(curr_size));
    std::memcpy(out.data(), &curr_size, sizeof(curr_size));

    size_t min_len = std::min(previous.size(), current.size());
    for (size_t i = 0; i < min_len; ++i) {
        out.push_back(current[i] - previous[i]); // Wraparound is fine and deterministic
    }

    for (size_t i = min_len; i < current.size(); ++i) {
        out.push_back(current[i]);
    }

    return out;
}

std::vector<uint8_t> DeterministicCompressor::delta_decode(const std::vector<uint8_t>& previous, const std::vector<uint8_t>& delta) {
    if (delta.size() < sizeof(uint64_t)) {
        throw std::runtime_error("Delta decode failed: Insufficient data");
    }

    uint64_t curr_size;
    std::memcpy(&curr_size, delta.data(), sizeof(curr_size));

    if (delta.size() - sizeof(uint64_t) != curr_size) {
        throw std::runtime_error("Delta decode failed: Size mismatch");
    }

    std::vector<uint8_t> out;
    out.reserve(curr_size);

    size_t offset = sizeof(uint64_t);
    size_t min_len = std::min(previous.size(), static_cast<size_t>(curr_size));

    for (size_t i = 0; i < min_len; ++i) {
        out.push_back(delta[offset + i] + previous[i]);
    }

    for (size_t i = min_len; i < curr_size; ++i) {
        out.push_back(delta[offset + i]);
    }

    return out;
}

std::vector<uint8_t> DeterministicCompressor::rle_encode(const std::vector<uint8_t>& raw) {
    std::vector<uint8_t> out;
    if (raw.empty()) return out;

    size_t i = 0;
    while (i < raw.size()) {
        uint8_t count = 1;
        while (i + count < raw.size() && raw[i + count] == raw[i] && count < 255) {
            count++;
        }
        out.push_back(count);
        out.push_back(raw[i]);
        i += count;
    }
    return out;
}

std::vector<uint8_t> DeterministicCompressor::rle_decode(const std::vector<uint8_t>& raw) {
    std::vector<uint8_t> out;
    if (raw.size() % 2 != 0) {
        throw std::runtime_error("RLE decode failed: Invalid format");
    }

    for (size_t i = 0; i < raw.size(); i += 2) {
        uint8_t count = raw[i];
        uint8_t val = raw[i + 1];
        for (uint8_t c = 0; c < count; ++c) {
            out.push_back(val);
        }
    }
    return out;
}

std::vector<uint8_t> DeterministicCompressor::stable_hash(const std::vector<uint8_t>& raw) {
    std::vector<uint8_t> out = raw;
    uint32_t hash = fnv1a_32(raw);

    out.resize(out.size() + sizeof(hash));
    std::memcpy(out.data() + raw.size(), &hash, sizeof(hash));

    return out;
}

} // namespace l5
} // namespace ailee
