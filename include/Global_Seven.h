// Global_Seven.h
// AILEE‑Core: Multi‑chain adapter interface + financial engineering layer
// Goal: Scale adapters beyond seven chains, normalize value, and anchor settlements to Bitcoin L1.
//
// Design pillars:
// 1) Bitcoin‑anchored: All irreversible settlements default to BTC L1 primitives (multisig, time‑locks).
// 2) Chain‑agnostic engine + chain‑specific adapters (pluggable registry).
// 3) Financial controls: slippage caps, fee policies, oracle confidence, and circuit‑breaker flags.
// 4) Deterministic normalization: units, decimals, and value accounting across heterogeneous chains.

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <cmath>

namespace ailee::l1 {
class AILEEMempoolAdapter;
class AILEENetworkAdapter;
class AILEEEnergyAdapter;
} // namespace ailee::l1

namespace ailee {
namespace global_seven {
using ::ailee::l1::AILEEEnergyAdapter;
using ::ailee::l1::AILEEMempoolAdapter;
using ::ailee::l1::AILEENetworkAdapter;

struct ETHState;
struct BTCState;

// ---------- Canonical chain set (extensible) ----------

enum class Chain {
    // Bitcoin family
    Bitcoin,
    Litecoin,
    Dogecoin,
    BitcoinCash,

    // EVM / Smart contracts
    Ethereum,
    Polygon,
    Arbitrum,
    Optimism,
    BNBChain,

    // Non‑EVM high‑throughput
    Solana,
    Avalanche,
    Near,
    Aptos,

    // UTXO/alt designs
    Cardano,
    Monero,        // note: limited introspection by design
    Dash,

    // Substrate / modular
    Polkadot,
    Kusama,

    // Reserve slots (custom forks, testnets, future chains)
    Custom1,
    Custom2
};

enum class Severity { Debug, Info, Warn, Error, Critical };

// ---------- Diagnostics ----------

struct AdapterError {
    Severity    severity{Severity::Error};
    std::string message;
    std::string component;   // "RPC", "Listener", "Bridge", "Oracle"
    int         code{-1};
    std::chrono::system_clock::time_point when{std::chrono::system_clock::now()};
};

// ---------- Monetary normalization ----------

struct UnitSpec {
    // Smallest unit per chain (e.g., sats for BTC, wei for ETH, lamports for SOL)
    uint8_t   decimals{8};              // precision used for normalization
    std::string unitName;               // "sats", "wei", "lamports"
    std::string displayTicker;          // "BTC", "ETH", "SOL"
};

// Canonical price/amount with deterministic decimals across chains
struct Amount {
    Chain      chain;
    UnitSpec   unit;
    // store in smallest units (integer) to avoid FP risk
    uint64_t   smallestUnits{0};

    // Helper: convert to display units as double (for UI), not for settlement math
    double toDisplay() const {
        return static_cast<double>(smallestUnits) / std::pow(10.0, unit.decimals);
    }
};

// ---------- Transaction and block primitives ----------

struct TxIn {
    std::string txid;
    uint32_t    index{0};
    std::string scriptOrData;  // UTXO scriptPubKey or call data
    Amount      amount;
};

struct TxOut {
    std::string address;
    Amount      amount;
    std::optional<std::string> memo;    // peg proofs, reference tags
};

struct NormalizedTx {
    std::string chainTxId;              // native id
    std::string normalizedId;           // canonical id for cross‑chain
    Chain       chain;
    bool        confirmed{false};
    uint32_t    confirmations{0};
    std::vector<TxIn>  inputs;
    std::vector<TxOut> outputs;
    std::unordered_map<std::string, std::string> metadata; // hints: vaultId, pegTag, oracleStamp
};

// ---------- Taproot Challenge-Response (BitVM-style) ----------
struct TapLeaf {
    uint8_t leafVersion;
    std::vector<uint8_t> script;
    std::string scriptHex; // For debugging

    // Hash of this leaf according to BIP 340 / BIP 341
    std::string getTapLeafHash() const;
};

struct TapTree {
    std::vector<TapLeaf> leaves;
    std::string rootHash;

    // Build a simple MAST root from leaves
    void computeRoot();
};

// ---------- Deterministic L2 anchor commitment ----------
struct AnchorCommitment {
    std::string l2StateRoot;
    uint64_t timestampMs{0};
    std::string recoveryMetadata;
    std::string payload;
    std::string hash;

    struct AnchorPayload {
        std::vector<uint8_t> scriptBytes;
        std::string description;
    };

    AnchorPayload buildOpReturnPayload() const;
    AnchorPayload buildTaprootCommitment() const;

    // Build a BitVM-style Taproot challenge-response tree
    TapTree buildChallengeResponseTree(const std::string& zkProofHash) const;
};

struct BlockHeader {
    std::string  hash;
    uint64_t     height{0};
    std::string  parentHash;
    std::chrono::system_clock::time_point timestamp;
    Chain        chain;
};

// ---------- Telemetry ----------

struct EnergyTelemetry {
    double latencyMs{0.0};
    double nodeTempC{0.0};
    double energyEfficiencyScore{0.0};  // adapter‑specific metric (normalized to 0–100)
};

// ---------- Financial engineering controls ----------

struct FeePolicy {
    // Fees expressed in smallest units of the settlement chain (typically BTC sats)
    uint64_t baseFee{0};                // fixed component
    double   percentFee{0.0};           // e.g., 0.25% as 0.0025
    uint64_t maxFeeCap{0};              // absolute cap in smallest units
};

struct SlippagePolicy {
    // Maximum allowed difference between quoted and executed value
    double   maxSlippagePct{0.01};      // 1% default
    bool     enforceHard{true};
};

struct OracleSignal {
    std::string source;                 // "Chainlink", "Internal", "Custom"
    double      price;                  // display price (UI only)
    double      confidence{0.0};        // 0–1; circuit breakers may enforce minimum confidence
    std::chrono::system_clock::time_point asOf;
};

struct RiskFlags {
    bool circuitBreakerTripped{false};  // when true, restrict to BTC settlement only
    bool readOnlyMode{false};           // disallow broadcasts
    bool anomalyDetected{false};        // telemetry anomalies or oracle deviation
    std::string reason;
};

// ---------- Settlement intents (Bitcoin‑anchored by design) ----------

enum class SettlementKind {
    PegIn,              // lock on source chain, mint on L2
    PegOut,             // burn on L2, release from BTC vault
    SwapCrossChain,     // swap between chains via anchored proofs
    Checkpoint,         // write a state checkpoint on BTC L1
    FeeSweep            // consolidate fees to BTC vault
};

struct SettlementIntent {
    SettlementKind kind;
    Chain          sourceChain;
    Chain          targetChain;         // often Chain::Bitcoin for final settlement
    Amount         amountSource;
    Amount         minReceiveTarget;    // after slippage/fees
    FeePolicy      feePolicy;
    SlippagePolicy slippagePolicy;
    std::optional<OracleSignal> oracle;
    std::unordered_map<std::string, std::string> params; // vaultId, contractAddr, programId
};

// ---------- Adapter traits and capabilities ----------

struct AdapterTraits {
    bool supportsEvents{true};          // live tx/block feeds
    bool supportsBroadcast{true};
    bool supportsSmartContracts{false};
    bool supportsUTXO{false};
    bool supportsPrivacy{false};        // e.g., Monero: limited visibility
    UnitSpec defaultUnit;
    std::string adapterName;
    std::string version;
    bool audited{false};                // set true after internal/external review
};

// ---------- Configuration ----------

struct AdapterConfig {
    Chain       chain{Chain::Bitcoin};
    std::string nodeEndpoint;           // RPC/WS/IPC URL
    std::string authUsername;
    std::string authPassword;
    std::string network;                // "mainnet", "testnet", "devnet"
    std::unordered_map<std::string, std::string> extra; // per‑chain params
    bool        enableTelemetry{true};
    bool        readOnly{true};         // listen‑only (default: shadow/read-only mode)
    FeePolicy   feePolicy{};
    SlippagePolicy slippagePolicy{};
    double      minOracleConfidence{0.7};
};

// ---------- Callbacks ----------

using TxCallback     = std::function<void(const NormalizedTx&)>;
using BlockCallback  = std::function<void(const BlockHeader&)>;
using ErrorCallback  = std::function<void(const AdapterError&)>;
using EnergyCallback = std::function<void(const EnergyTelemetry&)>;

// ---------- Portfolio and accounting ----------

struct Position {
    Chain     chain;
    Amount    balance;
    double    lastPriceUI{0.0}; // UI only
};

struct Portfolio {
    std::vector<Position> positions;
    // Aggregate display value (UI only); settlements rely on Amount + oracle math elsewhere
    double totalDisplayValue() const {
        double sum = 0.0;
        for (const auto& p : positions) sum += p.balance.toDisplay() * p.lastPriceUI;
        return sum;
    }
};

// ---------- Adapter interface ----------

class IChainAdapter {
public:
    virtual ~IChainAdapter() = default;

    // Lifecycle
    virtual bool init(const AdapterConfig& cfg, ErrorCallback onError) = 0;
    virtual bool start(TxCallback onTx, BlockCallback onBlock, EnergyCallback onEnergy) = 0;
    virtual void stop() = 0;

    // Broadcast (peg‑outs, burns, checkpoints, swaps)
    virtual bool broadcastTransaction(const std::vector<TxOut>& outputs,
                                      const std::unordered_map<std::string, std::string>& opts,
                                      std::string& outChainTxId) = 0;

    // Query
    virtual std::optional<NormalizedTx>   getTransaction(const std::string& chainTxId) = 0;
    virtual std::optional<BlockHeader>    getBlockHeader(const std::string& blockHash) = 0;
    virtual std::optional<uint64_t>       getBlockHeight() = 0;

    // Traits + identity
    virtual Chain                         chain() const = 0;
    virtual AdapterTraits                 traits() const = 0;
};

// ---------- Registry (pluggable adapters) ----------

class AdapterRegistry {
public:
    static AdapterRegistry& instance();
    void registerAdapter(Chain chain, std::unique_ptr<IChainAdapter> adapter);
    IChainAdapter* get(Chain chain) const;

private:
    AdapterRegistry() = default;
    std::unordered_map<Chain, std::unique_ptr<IChainAdapter>> adapters_;
};

// ---------- Bitcoin‑anchored settlement orchestrator ----------

class SettlementOrchestrator {
public:
    explicit SettlementOrchestrator(ErrorCallback onError) : onError_(std::move(onError)) {}

    // Execute intent with risk controls; returns target chain tx id if broadcast occurs
    bool execute(const SettlementIntent& intent,
                 std::string& outTargetTxId,
                 RiskFlags& outRisk) {

        outRisk = currentRisk_;

        // Circuit breaker: force BTC settlement only
        if (currentRisk_.circuitBreakerTripped) {
            if (intent.targetChain != Chain::Bitcoin) {
                outRisk.reason = "Circuit breaker: non‑BTC settlement blocked.";
                return false;
            }
        }

        // Oracle confidence check (if provided)
        if (intent.oracle.has_value() && intent.oracle->confidence < minOracleConfidence_) {
            outRisk.anomalyDetected = true;
            outRisk.reason = "Low oracle confidence.";
            if (enforceOracleConfidence_) return false;
        }

        // Fee/slippage pre‑check (display‑level validation; exact math in adapters)
        // Note: actual settlement math must be integer‑safe and done per chain adapter.
        if (intent.slippagePolicy.enforceHard && intent.slippagePolicy.maxSlippagePct <= 0.0) {
            outRisk.reason = "Invalid slippage policy.";
            return false;
        }

        // Route to target adapter (often BTC for final settlement)
        IChainAdapter* adapter = AdapterRegistry::instance().get(intent.targetChain);
        if (!adapter) {
            outRisk.reason = "No adapter registered for target chain.";
            return false;
        }

        // Build outputs (simplified; actual peg logic handled upstream)
        std::vector<TxOut> outs;
        TxOut o;
        o.address = intent.params.count("targetAddress") ? intent.params.at("targetAddress") : "";
        o.amount  = intent.minReceiveTarget;
        outs.push_back(o);

        std::unordered_map<std::string, std::string> opts = {
            {"settlementKind", std::to_string(static_cast<int>(intent.kind))},
            {"vaultId", intent.params.count("vaultId") ? intent.params.at("vaultId") : ""},
            {"pegTag", intent.params.count("pegTag") ? intent.params.at("pegTag") : ""}
        };

        // Broadcast via target adapter
        std::string txid;
        bool ok = adapter->broadcastTransaction(outs, opts, txid);
        if (!ok) {
            outRisk.reason = "Broadcast failed at target adapter.";
            currentRisk_.anomalyDetected = true;
            onError_(AdapterError{Severity::Error, "Broadcast failure", "Orchestrator", -2});
            return false;
        }

        outTargetTxId = txid;
        return true;
    }

    void setRisk(const RiskFlags& r) { currentRisk_ = r; }
    RiskFlags getRisk() const { return currentRisk_; }

    void setOracleConfidenceFloor(double floor, bool enforce) {
        minOracleConfidence_ = floor;
        enforceOracleConfidence_ = enforce;
    }

private:
    ErrorCallback onError_;
    RiskFlags     currentRisk_{};
    double        minOracleConfidence_{0.7};
    bool          enforceOracleConfidence_{true};
};

// ---------- Example adapter stubs (names only; implement in .cpp) ----------

class BitcoinAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    AnchorCommitment buildAnchorCommitment(const std::string& l2StateRoot,
                                           uint64_t timestampMs,
                                           const std::string& recoveryMetadata) const;
    Chain chain() const override { return Chain::Bitcoin; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            /*supportsEvents*/true, /*supportsBroadcast*/true,
            /*supportsSmartContracts*/false, /*supportsUTXO*/true, /*supportsPrivacy*/false,
            UnitSpec{8, "sats", "BTC"}, "BitcoinAdapter", "1.0.0", /*audited*/true
        };
    }

    void attachMempoolAdapter(std::unique_ptr<AILEEMempoolAdapter> adapter);
    void attachNetworkAdapter(std::unique_ptr<AILEENetworkAdapter> adapter);
    void attachEnergyAdapter(std::unique_ptr<AILEEEnergyAdapter> adapter);

private:
    static std::shared_ptr<BTCState> state_;
};

class EthereumAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Ethereum; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, true, false, false,
            UnitSpec{18, "wei", "ETH"}, "EthereumAdapter", "1.0.0", false
        };
    }

private:
    std::shared_ptr<ETHState> state_;
};

class LitecoinAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Litecoin; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, false, true, false,
            UnitSpec{8, "litoshi", "LTC"}, "LitecoinAdapter", "1.0.0", false
        };
    }
};

class DogecoinAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Dogecoin; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, false, true, false,
            UnitSpec{8, "koinu", "DOGE"}, "DogecoinAdapter", "1.0.0", false
        };
    }
};

class PolygonAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Polygon; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, true, false, false,
            UnitSpec{18, "wei", "MATIC"}, "PolygonAdapter", "1.0.0", false
        };
    }
};

class SolanaAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Solana; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, true, false, false,
            UnitSpec{9, "lamports", "SOL"}, "SolanaAdapter", "1.0.0", false
        };
    }
};

class AvalancheAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Avalanche; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, true, false, false,
            UnitSpec{18, "wei", "AVAX"}, "AvalancheAdapter", "1.0.0", false
        };
    }
};

class CardanoAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Cardano; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, true, false, false,
            UnitSpec{6, "lovelace", "ADA"}, "CardanoAdapter", "1.0.0", false
        };
    }
};

class PolkadotAdapter final : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override;
    bool start(TxCallback, BlockCallback, EnergyCallback) override;
    void stop() override;
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string&) override;
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override;
    std::optional<BlockHeader>  getBlockHeader(const std::string& blockHash) override;
    std::optional<uint64_t>     getBlockHeight() override;
    Chain chain() const override { return Chain::Polkadot; }
    AdapterTraits traits() const override {
        return AdapterTraits{
            true, true, true, false, false,
            UnitSpec{10, "plancks", "DOT"}, "PolkadotAdapter", "1.0.0", false
        };
    }
};

} // namespace global_seven
} // namespace ailee
