#include <array>
#ifndef AMBIENT_AI_BITCOIN_NODE_VIEW_HPP
#define AMBIENT_AI_BITCOIN_NODE_VIEW_HPP

#include <vector>
#include <map>
#include <memory>

namespace ailee {
namespace bitcoin {

using Hash256 = std::array<uint8_t, 32>;

// Canonical representation of a validated Bitcoin block header
struct BlockHeader {
    int32_t version;
    Hash256 prevBlockHash;
    Hash256 merkleRoot;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
    Hash256 blockHash; // Cached self-hash (double SHA-256)
};

// Represents a deterministic, raw transaction view
struct RawTransaction {
    Hash256 txId;
    std::vector<uint8_t> serializedBytes;
    bool hasWitness;
};

// Represents the deterministically verified Merkle Proof of inclusion
struct MerkleProof {
    Hash256 txId;
    Hash256 blockHash;
    std::vector<Hash256> siblingHashes;
    uint32_t txIndex;

    // Validates the proof against a known block header Merkle root
    bool verify(const Hash256& expectedMerkleRoot) const;
};

// Abstract interface for the deterministic L1 View
class IBitcoinNodeView {
public:
    virtual ~IBitcoinNodeView() = default;

    // Returns the current confirmed tip of the Bitcoin chain
    virtual BlockHeader getChainTip() const = 0;

    // Fetches a specific header deterministically. Throws if missing.
    virtual BlockHeader getHeader(const Hash256& blockHash) const = 0;
    virtual BlockHeader getHeaderByHeight(uint64_t height) const = 0;

    // Checks if an epoch anchor commitment exists within a specific block
    virtual bool containsTaprootCommitment(
        const Hash256& blockHash,
        const Hash256& expectedCommitment
    ) const = 0;

    // Stream subscriptions (ZMQ backed)
    // Implementations must strictly sequence events.
    class IBlockSubscriber {
    public:
        virtual ~IBlockSubscriber() = default;
        virtual void onBlockConnected(const BlockHeader& header, const std::vector<RawTransaction>& txs) = 0;
        virtual void onBlockDisconnected(const BlockHeader& header) = 0; // Reorg handling
    };

    virtual void subscribe(std::shared_ptr<IBlockSubscriber> subscriber) = 0;
};

} // namespace bitcoin
} // namespace ailee

#endif // AMBIENT_AI_BITCOIN_NODE_VIEW_HPP
