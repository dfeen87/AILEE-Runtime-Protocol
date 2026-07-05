#ifndef AMBIENT_AI_NODE_IDENTITY_HPP
#define AMBIENT_AI_NODE_IDENTITY_HPP

#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace ailee {
namespace identity {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic metadata reflecting the node's compilation environment.
// These values are injected via CMake preprocessor macros from BuildInfo.hpp.
struct BuildMetadata {
    std::string commitHash;      // 40-character git commit hex string
    uint32_t buildNumber;        // Monotonically increasing build ID
    uint32_t protocolVersion;    // Strict protocol consensus version

    // Returns the deterministic SHA-256 hash of the serialized metadata.
    Hash256 canonicalHash() const;
};

// The core cryptographic identity payload that a node signs to participate.
struct IdentityPayload {
    std::string peerId;          // libp2p Peer ID (base58 or multihash)
    std::string publicKeyHex;    // secp256k1 public key in uncompressed hex format (130 chars)
    BuildMetadata metadata;      // The deterministic build signature
    uint64_t epochId;            // The epoch this identity is committing to
    uint64_t staticAttribute;    // Optional protocol-defined static attribute (e.g., node type)

    // Serializes the payload into a strictly deterministic byte array.
    std::vector<uint8_t> serialize() const;

    // Computes the SHA-256 hash over the serialized payload.
    // Must strictly use libsecp256k1 equivalent (e.g., via secp256k1_sha256).
    Hash256 hash() const;
};

// The participation proof submitted by the node to the mesh network.
struct ParticipationProof {
    IdentityPayload payload;
    std::vector<uint8_t> ecdsaSignature; // Strict DER-encoded secp256k1 signature over payload.hash()

    // Verifies the ECDSA signature against the payload's public key.
    // Also validates that protocolVersion matches the current consensus requirement.
    bool verify() const;
};

} // namespace identity
} // namespace ailee

#endif // AMBIENT_AI_NODE_IDENTITY_HPP
