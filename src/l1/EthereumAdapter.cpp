// EthereumAdapter.cpp
#include "Global_Seven.h"
#include <thread>
#include <atomic>
#include <sstream>
#include <memory>
#include "JsonRpcClient.h"

namespace ailee {
namespace global_seven {

class ETHInternal {
public:
    bool connectRPC(const AdapterConfig& cfg, ErrorCallback onError) {
        rpcClient_ = std::make_unique<JsonRpcClient>(cfg.nodeEndpoint,
                                                     cfg.authUsername,
                                                     cfg.authPassword);
        rpcEndpoint_ = cfg.nodeEndpoint;
        auto resp = rpcClient_->call("eth_chainId", nlohmann::json::array(), onError);
        if (!resp || !resp->contains("result")) {
            return false;
        }
        auto parsed = parseHexU64((*resp)["result"].get<std::string>());
        if (!parsed.has_value()) return false;
        chainId_ = parsed.value();
        return true;
    }
    bool connectWS(const std::string& endpoint) {
        wsEndpoint_ = endpoint;
        if (wsEndpoint_.rfind("ws://", 0) != 0 && wsEndpoint_.rfind("wss://", 0) != 0) {
            return false;
        }
        connectedWS_ = true;
        return true;
    }
    bool sendRawTx(const std::string& rawHex, std::string& outTxHash, ErrorCallback onError) {
        if (!rpcClient_) return false;
        auto resp = rpcClient_->call("eth_sendRawTransaction",
                                     nlohmann::json::array({rawHex}),
                                     onError);
        if (!resp || !resp->contains("result")) return false;
        outTxHash = (*resp)["result"].get<std::string>();
        return true;
    }
    std::optional<NormalizedTx> getTx(const std::string& hash, ErrorCallback onError) {
        if (!rpcClient_) return std::nullopt;
        auto resp = rpcClient_->call("eth_getTransactionByHash",
                                     nlohmann::json::array({hash}),
                                     onError);
        if (!resp || !resp->contains("result") || (*resp)["result"].is_null()) {
            return std::nullopt;
        }
        const auto& tx = (*resp)["result"];
        auto receipt = rpcClient_->call("eth_getTransactionReceipt",
                                        nlohmann::json::array({hash}),
                                        onError);
        NormalizedTx nt;
        nt.chainTxId = hash;
        nt.normalizedId = hash;
        nt.chain = Chain::Ethereum;
        if (tx.contains("blockNumber") && !tx["blockNumber"].is_null()) {
            nt.confirmed = true;
        }
        if (receipt && receipt->contains("result") && !(*receipt)["result"].is_null()) {
            if ((*receipt)["result"].contains("status")) {
                nt.metadata["status"] = (*receipt)["result"]["status"].get<std::string>();
            }
        }
        return nt;
    }
    std::optional<BlockHeader> getHeader(const std::string& hash, ErrorCallback onError) {
        if (!rpcClient_) return std::nullopt;
        auto resp = rpcClient_->call("eth_getBlockByHash",
                                     nlohmann::json::array({hash, false}),
                                     onError);
        if (!resp || !resp->contains("result") || (*resp)["result"].is_null()) {
            return std::nullopt;
        }
        const auto& block = (*resp)["result"];
        BlockHeader bh;
        bh.hash = hash;
        if (block.contains("number") && block["number"].is_string()) {
            auto parsed = parseHexU64(block["number"].get<std::string>());
            if (parsed.has_value()) bh.height = parsed.value();
        }
        if (block.contains("parentHash")) bh.parentHash = block["parentHash"].get<std::string>();
        if (block.contains("timestamp")) {
            auto parsed = parseHexU64(block["timestamp"].get<std::string>());
            if (parsed.has_value()) bh.timestamp = fromUnixSeconds(parsed.value());
        }
        bh.chain = Chain::Ethereum;
        return bh;
    }
    std::optional<uint64_t> height() {
        if (!rpcClient_) return std::nullopt;
        auto resp = rpcClient_->call("eth_blockNumber", nlohmann::json::array(), nullptr);
        if (!resp || !resp->contains("result")) return std::nullopt;
        return parseHexU64((*resp)["result"].get<std::string>());
    }

private:
    std::string rpcEndpoint_;
    std::string wsEndpoint_;
    uint64_t chainId_{0};
    bool connectedWS_{false};
    std::unique_ptr<JsonRpcClient> rpcClient_;
};

struct ETHState {
    AdapterConfig cfg;
    ErrorCallback onError;
    TxCallback onTx;
    BlockCallback onBlock;
    EnergyCallback onEnergy;
    std::atomic<bool> running{false};
    std::thread eventThread;
    ETHInternal internal;
};

bool EthereumAdapter::init(const AdapterConfig& cfg, ErrorCallback onError) {
    state_ = std::make_shared<ETHState>();
    state_->cfg = cfg;
    state_->onError = onError;

    if (!state_->internal.connectRPC(cfg, onError)) {
        onError(AdapterError{Severity::Error, "ETH RPC connect failed", "RPC", -1});
        return false;
    }

    auto it = cfg.extra.find("ws");
    if (it != cfg.extra.end()) {
        if (!state_->internal.connectWS(it->second)) {
            onError(AdapterError{Severity::Warn, "ETH WS connect failed; falling back to poll", "Listener", -2});
        }
    }
    return true;
}

bool EthereumAdapter::start(TxCallback onTx, BlockCallback onBlock, EnergyCallback onEnergy) {
    if (!state_) return false;
    state_->onTx = onTx;
    state_->onBlock = onBlock;
    state_->onEnergy = onEnergy;
    state_->running.store(true);

    state_->eventThread = std::thread([s = state_]() {
        using namespace std::chrono_literals;
        auto lastEnergy = std::chrono::steady_clock::now();

        while (s->running.load()) {
            auto h = s->internal.height();
            if (h.has_value()) {
                BlockHeader bh;
                bh.hash = "eth_dummy_hash_" + std::to_string(h.value());
                bh.height = h.value();
                bh.parentHash = "eth_dummy_parent";
                bh.timestamp = std::chrono::system_clock::now();
                bh.chain = Chain::Ethereum;
                if (s->onBlock) s->onBlock(bh);
            }

            if (std::chrono::steady_clock::now() - lastEnergy > 5s && s->cfg.enableTelemetry) {
                EnergyTelemetry et;
                et.latencyMs = 12.0;
                et.nodeTempC = 47.0;
                et.energyEfficiencyScore = 82.0;
                if (s->onEnergy) s->onEnergy(et);
                lastEnergy = std::chrono::steady_clock::now();
            }

            std::this_thread::sleep_for(1s);
        }
    });

    return true;
}

void EthereumAdapter::stop() {
    if (!state_) return;
    state_->running.store(false);
    if (state_->eventThread.joinable()) state_->eventThread.join();
}

bool EthereumAdapter::broadcastTransaction(const std::vector<TxOut>& outputs,
                                           const std::unordered_map<std::string, std::string>& opts,
                                           std::string& outChainTxId) {
    (void)outputs;
    if (!state_) return false;
    if (state_->cfg.readOnly) {
        state_->onError(AdapterError{Severity::Warn, "Read‑only mode; broadcast blocked", "Broadcast", -10});
        return false;
    }

    auto it = opts.find("raw_tx");
    std::string rawHex;
    if (it != opts.end()) {
        rawHex = it->second;
    } else if ((it = opts.find("signed_tx")) != opts.end()) {
        rawHex = it->second;
    } else {
        state_->onError(AdapterError{Severity::Error,
                                     "Missing signed transaction hex in opts (raw_tx or signed_tx)",
                                     "Broadcast",
                                     -11});
        return false;
    }

    bool ok = state_->internal.sendRawTx(rawHex, outChainTxId, state_->onError);
    if (!ok) {
        state_->onError(AdapterError{Severity::Error, "ETH broadcast failed", "Broadcast", -11});
        return false;
    }
    return true;
}

std::optional<NormalizedTx> EthereumAdapter::getTransaction(const std::string& chainTxId) {
    if (!state_) return std::nullopt;
    return state_->internal.getTx(chainTxId, state_->onError);
}

std::optional<BlockHeader> EthereumAdapter::getBlockHeader(const std::string& blockHash) {
    if (!state_) return std::nullopt;
    return state_->internal.getHeader(blockHash, state_->onError);
}

std::optional<uint64_t> EthereumAdapter::getBlockHeight() {
    if (!state_) return std::nullopt;
    return state_->internal.height();
}

} // namespace global_seven
} // namespace ailee
