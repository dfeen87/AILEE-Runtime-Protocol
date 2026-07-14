#include "L2State.h"

#include "Ledger.h"
#include "Orchestrator.h"
#include "ailee_sidechain_bridge.h"
#include "zk_proofs.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace ailee::l2 {

namespace {

constexpr const char* kSnapshotHeader = "SNAPSHOT v1";
constexpr const char* kSnapshotEnd = "END_SNAPSHOT";

std::string quoted(const std::string& value) {
    std::ostringstream oss;
    oss << std::quoted(value);
    return oss.str();
}

bool readQuoted(std::istringstream& iss, std::string* out) {
    if (!out) return false;  // Validate output pointer
    std::string value;
    if (!(iss >> std::quoted(value))) {
        return false;
    }
    *out = value;
    return true;
}

std::vector<ailee::sched::TaskPayload> sortedTasks(const std::vector<ailee::sched::TaskPayload>& tasks) {
    auto sorted = tasks;
    std::sort(sorted.begin(), sorted.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.taskId < rhs.taskId;
    });
    return sorted;
}

void sortSnapshot(L2StateSnapshot& snapshot) {
    std::sort(snapshot.ledger.balances.begin(), snapshot.ledger.balances.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.peerId < rhs.peerId; });
    std::sort(snapshot.ledger.escrows.begin(), snapshot.ledger.escrows.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.taskId < rhs.taskId; });
    std::sort(snapshot.bridge.pegins.begin(), snapshot.bridge.pegins.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.pegId < rhs.pegId; });
    std::sort(snapshot.bridge.pegouts.begin(), snapshot.bridge.pegouts.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.pegId < rhs.pegId; });
    std::sort(snapshot.orchestration.tasks.begin(), snapshot.orchestration.tasks.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.taskId < rhs.taskId; });
}

std::string canonicalizeSnapshot(const L2StateSnapshot& snapshot) {
    std::ostringstream oss;
    oss << "L2STATE|v1\n";
    oss << "balances:" << snapshot.ledger.balances.size() << "\n";
    for (const auto& balance : snapshot.ledger.balances) {
        oss << "balance:" << balance.peerId << ":" << balance.balance << "\n";
    }
    oss << "escrows:" << snapshot.ledger.escrows.size() << "\n";
    for (const auto& escrow : snapshot.ledger.escrows) {
        oss << "escrow:" << escrow.taskId << ":" << escrow.clientPeerId << ":"
            << escrow.amount << ":" << (escrow.locked ? 1 : 0) << ":" << escrow.createdAt << "\n";
    }
    oss << "pegins:" << snapshot.bridge.pegins.size() << "\n";
    for (const auto& pegin : snapshot.bridge.pegins) {
        oss << "pegin:" << pegin.pegId << ":" << pegin.btcTxId << ":" << pegin.vout << ":"
            << pegin.btcAmount << ":" << pegin.btcSource << ":" << pegin.aileeDest << ":"
            << pegin.status << ":" << pegin.confirmations << ":" << pegin.initiatedTime << ":"
            << pegin.completedTime << "\n";
    }
    oss << "pegouts:" << snapshot.bridge.pegouts.size() << "\n";
    for (const auto& pegout : snapshot.bridge.pegouts) {
        oss << "pegout:" << pegout.pegId << ":" << pegout.aileeSource << ":" << pegout.btcDest << ":"
            << pegout.aileeBurnAmount << ":" << pegout.btcReleaseAmount << ":" << pegout.status
            << ":" << pegout.anchorCommitmentHash << ":" << pegout.initiatedTime << ":"
            << pegout.completedTime << "\n";
    }
    oss << "tasks:" << snapshot.orchestration.tasks.size() << "\n";
    for (const auto& task : snapshot.orchestration.tasks) {
        oss << "task:" << task.taskId << ":" << task.taskType << ":" << task.priority << ":"
            << task.submitterId << ":" << task.submittedAtMs << ":" << task.payloadHash << ":"
            << task.anchorCommitmentHash << "\n";
    }
    return oss.str();
}

std::string hashPayloadBytes(const std::vector<std::uint8_t>& payload) {
    if (payload.empty()) {
        return "";
    }
    std::string data(payload.begin(), payload.end());
    return ailee::zk::sha256Hex(data);
}

std::string toLower(std::string value) {
    for (auto& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

} // namespace

std::string computeL2StateRoot(const L2StateSnapshot& snapshot) {
    L2StateSnapshot sorted = snapshot;
    sortSnapshot(sorted);
    return ailee::zk::sha256Hex(canonicalizeSnapshot(sorted));
}

bool validateAnchorCommitment(const AnchorSnapshot& anchor,
                              const std::string& expectedStateRoot,
                              std::string* err) {
    if (anchor.l2StateRoot != expectedStateRoot) {
        if (err) {
            *err = "Anchor state root mismatch. expected=" + expectedStateRoot +
                   " anchor=" + anchor.l2StateRoot;
        }
        return false;
    }
    const std::string computedHash = ailee::zk::sha256Hex(anchor.payload);
    if (toLower(computedHash) != toLower(anchor.hash)) {
        if (err) {
            *err = "Anchor hash mismatch. expected=" + computedHash + " anchor=" + anchor.hash;
        }
        return false;
    }
    return true;
}

bool appendSnapshotToFile(const L2StateSnapshot& snapshot,
                          const std::string& path,
                          std::string* err) {
    std::ofstream out(path, std::ios::app);
    if (!out) {
        if (err) {
            *err = "Failed to open snapshot file for append: " + path;
        }
        return false;
    }

    out << kSnapshotHeader << "\n";
    out << "timestamp_ms " << snapshot.snapshotTimestampMs << "\n";
    out << "balances " << snapshot.ledger.balances.size() << "\n";
    for (const auto& balance : snapshot.ledger.balances) {
        out << "balance " << quoted(balance.peerId) << " " << balance.balance << "\n";
    }
    out << "escrows " << snapshot.ledger.escrows.size() << "\n";
    for (const auto& escrow : snapshot.ledger.escrows) {
        out << "escrow " << quoted(escrow.taskId) << " " << quoted(escrow.clientPeerId) << " "
            << escrow.amount << " " << (escrow.locked ? 1 : 0) << " " << escrow.createdAt << "\n";
    }
    out << "pegins " << snapshot.bridge.pegins.size() << "\n";
    for (const auto& pegin : snapshot.bridge.pegins) {
        out << "pegin " << quoted(pegin.pegId) << " " << quoted(pegin.btcTxId) << " " << pegin.vout
            << " " << pegin.btcAmount << " " << quoted(pegin.btcSource) << " "
            << quoted(pegin.aileeDest) << " " << pegin.status << " " << pegin.confirmations << " "
            << pegin.initiatedTime << " " << pegin.completedTime << "\n";
    }
    out << "pegouts " << snapshot.bridge.pegouts.size() << "\n";
    for (const auto& pegout : snapshot.bridge.pegouts) {
        out << "pegout " << quoted(pegout.pegId) << " " << quoted(pegout.aileeSource) << " "
            << quoted(pegout.btcDest) << " " << pegout.aileeBurnAmount << " "
            << pegout.btcReleaseAmount << " " << pegout.status << " "
            << quoted(pegout.anchorCommitmentHash) << " " << pegout.initiatedTime << " "
            << pegout.completedTime << "\n";
    }
    out << "tasks " << snapshot.orchestration.tasks.size() << "\n";
    for (const auto& task : snapshot.orchestration.tasks) {
        out << "task " << quoted(task.taskId) << " " << task.taskType << " " << task.priority << " "
            << quoted(task.submitterId) << " " << task.submittedAtMs << " "
            << quoted(task.payloadHash) << " " << quoted(task.anchorCommitmentHash) << "\n";
    }
    if (snapshot.anchor.has_value()) {
        const auto& anchor = snapshot.anchor.value();
        out << "anchor " << quoted(anchor.l2StateRoot) << " " << anchor.timestampMs << " "
            << quoted(anchor.recoveryMetadata) << " " << quoted(anchor.payload) << " "
            << quoted(anchor.hash) << "\n";
    } else {
        out << "anchor none\n";
    }
    out << kSnapshotEnd << "\n";

    if (!out) {
        if (err) {
            *err = "Failed to write snapshot file: " + path;
        }
        return false;
    }

    return true;
}

std::optional<L2StateSnapshot> loadLatestSnapshotFromFile(const std::string& path,
                                                          std::string* err) {
    std::ifstream in(path);
    if (!in) {
        if (err) {
            *err = "Failed to open snapshot file: " + path;
        }
        return std::nullopt;
    }

    std::string line;
    L2StateSnapshot current;
    bool inSnapshot = false;
    std::optional<L2StateSnapshot> latest;

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        if (line == kSnapshotHeader) {
            current = L2StateSnapshot{};
            inSnapshot = true;
            continue;
        }
        if (line == kSnapshotEnd && inSnapshot) {
            latest = current;
            inSnapshot = false;
            continue;
        }
        if (!inSnapshot) {
            continue;
        }

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;
        if (tag == "timestamp_ms") {
            iss >> current.snapshotTimestampMs;
        } else if (tag == "balance") {
            LedgerBalanceSnapshot balance;
            if (!readQuoted(iss, &balance.peerId)) {
                continue;
            }
            iss >> balance.balance;
            current.ledger.balances.push_back(balance);
        } else if (tag == "escrow") {
            LedgerEscrowSnapshot escrow;
            if (!readQuoted(iss, &escrow.taskId)) {
                continue;
            }
            if (!readQuoted(iss, &escrow.clientPeerId)) {
                continue;
            }
            int locked = 0;
            iss >> escrow.amount >> locked >> escrow.createdAt;
            escrow.locked = locked != 0;
            current.ledger.escrows.push_back(escrow);
        } else if (tag == "pegin") {
            PegInSnapshot pegin;
            if (!readQuoted(iss, &pegin.pegId)) {
                continue;
            }
            if (!readQuoted(iss, &pegin.btcTxId)) {
                continue;
            }
            iss >> pegin.vout >> pegin.btcAmount;
            if (!readQuoted(iss, &pegin.btcSource)) {
                continue;
            }
            if (!readQuoted(iss, &pegin.aileeDest)) {
                continue;
            }
            iss >> pegin.status >> pegin.confirmations >> pegin.initiatedTime >> pegin.completedTime;
            current.bridge.pegins.push_back(pegin);
        } else if (tag == "pegout") {
            PegOutSnapshot pegout;
            if (!readQuoted(iss, &pegout.pegId)) {
                continue;
            }
            if (!readQuoted(iss, &pegout.aileeSource)) {
                continue;
            }
            if (!readQuoted(iss, &pegout.btcDest)) {
                continue;
            }
            iss >> pegout.aileeBurnAmount >> pegout.btcReleaseAmount >> pegout.status;
            if (!readQuoted(iss, &pegout.anchorCommitmentHash)) {
                continue;
            }
            iss >> pegout.initiatedTime >> pegout.completedTime;
            current.bridge.pegouts.push_back(pegout);
        } else if (tag == "task") {
            TaskSnapshot task;
            if (!readQuoted(iss, &task.taskId)) {
                continue;
            }
            iss >> task.taskType >> task.priority;
            if (!readQuoted(iss, &task.submitterId)) {
                continue;
            }
            iss >> task.submittedAtMs;
            if (!readQuoted(iss, &task.payloadHash)) {
                continue;
            }
            if (!readQuoted(iss, &task.anchorCommitmentHash)) {
                continue;
            }
            current.orchestration.tasks.push_back(task);
        } else if (tag == "anchor") {
            iss >> std::ws;
            if (iss.peek() == 'n') {
                std::string marker;
                iss >> marker;
                current.anchor.reset();
            } else {
                AnchorSnapshot anchor;
                if (!readQuoted(iss, &anchor.l2StateRoot)) {
                    continue;
                }
                iss >> anchor.timestampMs;
                if (!readQuoted(iss, &anchor.recoveryMetadata)) {
                    continue;
                }
                if (!readQuoted(iss, &anchor.payload)) {
                    continue;
                }
                if (!readQuoted(iss, &anchor.hash)) {
                    continue;
                }
                current.anchor = anchor;
            }
        }
    }

    if (!latest.has_value() && err) {
        *err = "No snapshots found in file: " + path;
    }

    return latest;
}

L2StateSnapshot captureSnapshot(const ailee::econ::ILedger& ledger,
                                const ailee::SidechainBridge& bridge,
                                const ailee::sched::Engine& engine,
                                const std::optional<AnchorSnapshot>& anchor,
                                std::uint64_t timestampMs) {
    L2StateSnapshot snapshot;
    snapshot.snapshotTimestampMs = timestampMs;
    snapshot.ledger = ledger.snapshot();
    snapshot.bridge = bridge.snapshotBridgeState();

    auto tasks = sortedTasks(engine.getQueuedTasks());
    snapshot.orchestration.tasks.reserve(tasks.size());
    for (const auto& task : tasks) {
        TaskSnapshot record;
        record.taskId = task.taskId;
        record.taskType = static_cast<int>(task.taskType);
        record.priority = static_cast<int>(task.priority);
        record.submitterId = task.submitterId;
        record.submittedAtMs = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                task.submittedAt.time_since_epoch())
                .count());
        record.payloadHash = hashPayloadBytes(task.payloadBytes);
        if (task.anchorCommitmentHash.has_value()) {
            record.anchorCommitmentHash = task.anchorCommitmentHash.value();
        }
        snapshot.orchestration.tasks.push_back(record);
    }

    snapshot.anchor = anchor;
    sortSnapshot(snapshot);
    return snapshot;
}

} // namespace ailee::l2

namespace ailee::l2 {

bool L2StateDiff::verifyDiff(const std::string& zkProofData) const {
    if (priorStateRoot.empty() || newStateRoot.empty()) return false;

    // For a real DA layer, we verify the zk proof matches the computation of the diff
    ailee::zk::Proof p;
    p.proofData = zkProofData;
    // Public input should commit to both the prior and new state roots to ensure valid transition
    p.publicInput = priorStateRoot + ":" + newStateRoot;

    ailee::zk::ZKEngine engine;
    return engine.verifyHalo2Proof(p);
}

nlohmann::json L2StateDiff::toJson() const {
    // Convert serialized changes to hex string since the custom nlohmann::json doesn't support binary
    std::string hexChanges;
    hexChanges.reserve(serializedChanges.size() * 2);
    constexpr char hex[] = "0123456789abcdef";
    for (uint8_t b : serializedChanges) {
        hexChanges.push_back(hex[b >> 4]);
        hexChanges.push_back(hex[b & 0xf]);
    }

    nlohmann::json refs = nlohmann::json::array_t{};
    for (const auto& ref : proofRefs) {
        refs.push_back(ref);
    }

    nlohmann::json j = nlohmann::json();
    j["height"] = static_cast<double>(height);
    j["priorStateRoot"] = priorStateRoot;
    j["newStateRoot"] = newStateRoot;
    j["diffPayload"] = hexChanges;
    j["proofRefs"] = refs;
    j["timestampMs"] = static_cast<double>(timestampMs);
    return j;
}

L2StateDiff L2StateDiff::fromJson(const nlohmann::json& j) {
    L2StateDiff diff;
    if (j.contains("height")) diff.height = j["height"].get<std::uint64_t>();
    if (j.contains("priorStateRoot")) diff.priorStateRoot = j["priorStateRoot"].get<std::string>();
    if (j.contains("newStateRoot")) diff.newStateRoot = j["newStateRoot"].get<std::string>();

    if (j.contains("diffPayload") && j["diffPayload"].is_string()) {
        std::string hexChanges = j["diffPayload"].get<std::string>();
        if (hexChanges.length() % 2 == 0) {
            for (size_t i = 0; i < hexChanges.length(); i += 2) {
                std::string byteString = hexChanges.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
                diff.serializedChanges.push_back(byte);
            }
        }
    }

    if (j.contains("proofRefs") && j["proofRefs"].is_array()) {
        for (const auto& refJson : j["proofRefs"]) {
             if (refJson.is_string()) {
                 diff.proofRefs.push_back(refJson.get<std::string>());
             }
        }
    }

    if (j.contains("timestampMs")) diff.timestampMs = j["timestampMs"].get<std::uint64_t>();
    return diff;
}

L2StateDiff calculateStateDiff(const L2StateSnapshot& oldState, const L2StateSnapshot& newState, std::uint64_t height) {
    L2StateDiff diff;
    diff.height = height;
    diff.timestampMs = newState.snapshotTimestampMs;
    diff.priorStateRoot = computeL2StateRoot(oldState);
    diff.newStateRoot = computeL2StateRoot(newState);

    // In a full implementation, we'd iterate the ledgers/escrows and record only changes.
    // For now, we simulate a serialized byte payload of changes.
    std::string mockDiff = "DIFF_DATA:" + diff.priorStateRoot + "->" + diff.newStateRoot;
    diff.serializedChanges.assign(mockDiff.begin(), mockDiff.end());

    // Proof refs would be generated based on the transactions included
    // diff.proofRefs.push_back("...");

    return diff;
}

bool applyStateDiff(L2StateSnapshot& currentState, const L2StateDiff& diff) {
    std::string currentRoot = computeL2StateRoot(currentState);
    if (currentRoot != diff.priorStateRoot) {
        return false; // State mismatch, reject the diff
    }

    // Placeholder for applying differential logic (e.g., updating balances/tasks)
    // Full logic requires unpacking diff.serializedChanges.

    // Advance timestamp heuristically to denote state change if we don't have exactly the target state
    currentState.snapshotTimestampMs = diff.timestampMs > 0 ? diff.timestampMs : currentState.snapshotTimestampMs + 1000;

    return true;
}

} // namespace ailee::l2
