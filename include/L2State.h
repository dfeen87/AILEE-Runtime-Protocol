#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ailee {
class SidechainBridge;
}

namespace ailee::econ {
class ILedger;
}

namespace ailee::sched {
class Engine;
struct TaskPayload;
}

namespace ailee::l2 {

struct LedgerBalanceSnapshot {
    std::string peerId;
    std::uint64_t balance{0};
};

struct LedgerEscrowSnapshot {
    std::string taskId;
    std::string clientPeerId;
    std::uint64_t amount{0};
    bool locked{false};
    std::uint64_t createdAt{0};
};

struct LedgerSnapshot {
    std::vector<LedgerBalanceSnapshot> balances;
    std::vector<LedgerEscrowSnapshot> escrows;
};

struct PegInSnapshot {
    std::string pegId;
    std::string btcTxId;
    std::uint32_t vout{0};
    std::uint64_t btcAmount{0};
    std::string btcSource;
    std::string aileeDest;
    std::uint64_t confirmations{0};
    std::uint64_t initiatedTime{0};
    std::uint64_t completedTime{0};
    int status{0};
};

struct PegOutSnapshot {
    std::string pegId;
    std::string aileeSource;
    std::string btcDest;
    std::uint64_t aileeBurnAmount{0};
    std::uint64_t btcReleaseAmount{0};
    std::uint64_t initiatedTime{0};
    std::uint64_t completedTime{0};
    int status{0};
    std::string anchorCommitmentHash;
};

struct BridgeSnapshot {
    std::vector<PegInSnapshot> pegins;
    std::vector<PegOutSnapshot> pegouts;
};

struct TaskSnapshot {
    std::string taskId;
    int taskType{0};
    int priority{0};
    std::string submitterId;
    std::uint64_t submittedAtMs{0};
    std::string payloadHash;
    std::string anchorCommitmentHash;
};

struct OrchestrationSnapshot {
    std::vector<TaskSnapshot> tasks;
};

struct AnchorSnapshot {
    std::string l2StateRoot;
    std::uint64_t timestampMs{0};
    std::string recoveryMetadata;
    std::string payload;
    std::string hash;
};

struct L2StateSnapshot {
    std::uint64_t snapshotTimestampMs{0};
    LedgerSnapshot ledger;
    BridgeSnapshot bridge;
    OrchestrationSnapshot orchestration;
    std::optional<AnchorSnapshot> anchor;
};

// Represents a deterministic difference between two L2StateSnapshots,
// to be broadcasted to decentralized Data Availability layers.
struct L2StateDiff {
    std::string priorStateRoot;
    std::string newStateRoot;
    std::vector<uint8_t> serializedChanges;

    // Validates the diff by verifying the attached ZK proof
    bool verifyDiff(const std::string& zkProofData) const;
};

std::string computeL2StateRoot(const L2StateSnapshot& snapshot);

// Calculates the differential changes between two states
L2StateDiff calculateStateDiff(const L2StateSnapshot& oldState, const L2StateSnapshot& newState);

// Applies a validated state diff to advance an L2StateSnapshot
bool applyStateDiff(L2StateSnapshot& currentState, const L2StateDiff& diff);


bool validateAnchorCommitment(const AnchorSnapshot& anchor,
                              const std::string& expectedStateRoot,
                              std::string* err);

bool appendSnapshotToFile(const L2StateSnapshot& snapshot,
                          const std::string& path,
                          std::string* err);

std::optional<L2StateSnapshot> loadLatestSnapshotFromFile(const std::string& path,
                                                          std::string* err);

L2StateSnapshot captureSnapshot(const ailee::econ::ILedger& ledger,
                                const ailee::SidechainBridge& bridge,
                                const ailee::sched::Engine& engine,
                                const std::optional<AnchorSnapshot>& anchor,
                                std::uint64_t timestampMs);

} // namespace ailee::l2

