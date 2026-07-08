// SPDX-License-Identifier: MIT
// AILEE Web Server - REST API for global web integration

#ifndef AILEE_WEB_SERVER_H
#define AILEE_WEB_SERVER_H

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>

namespace ailee {

// Forward declarations to avoid heavy includes
class Orchestrator;
class Ledger;

namespace l2 {
class BlockProducer;
class Mempool;
}

struct WebServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    bool enable_cors = true;
    bool enable_ssl = false;
    std::string ssl_cert_path;
    std::string ssl_key_path;
    int thread_pool_size = 4;
    std::string api_key; // Optional API key for authentication
};

struct NodeStatus {
    bool running;
    std::string version;
    uint64_t uptime_seconds;
    uint64_t total_transactions;
    uint64_t total_blocks;
    std::string network;
    double current_tps;
    size_t pending_tasks;
    std::string last_anchor_hash;
};

class AILEEWebServer {
public:
    AILEEWebServer(const WebServerConfig& config);
    ~AILEEWebServer();

    // Start the web server in a separate thread
    bool start();
    
    // Stop the web server
    void stop();
    
    // Check if server is running
    bool isRunning() const;

    // Set callbacks to get live data from the node
    void setNodeStatusCallback(std::function<NodeStatus()> callback);
    void setOrchestratorRef(Orchestrator* orch);
    void setLedgerRef(Ledger* ledger);
    void setBlockProducerRef(l2::BlockProducer* producer);
    void setMempoolRef(l2::Mempool* mempool);

    // Phase 16 External Bindings endpoints
    void setReplayTickCallback(std::function<std::string(uint64_t)> callback);
    void setFederationViewCallback(std::function<std::string()> callback);

    // V17 Telemetry Endpoints
    void setSyncEventsCallback(std::function<std::string()> callback);
    void setSyncClockCallback(std::function<std::string()> callback);
    void setLatestReplayTickCallback(std::function<std::string()> callback);
    void setMeshEnvelopesCallback(std::function<std::string()> callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    WebServerConfig config_;
};

} // namespace ailee

#endif // AILEE_WEB_SERVER_H
