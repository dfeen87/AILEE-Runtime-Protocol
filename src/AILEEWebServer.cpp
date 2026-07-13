// SPDX-License-Identifier: MIT
// AILEE Web Server Implementation - REST API for global web integration

#include "AILEEWebServer.h"
#include "Orchestrator.h"
#include "Ledger.h"
#include "L2State.h"
#include "BlockProducer.h"
#include "Mempool.h"
#include "third_party/httplib.h"
#include "nlohmann/json.hpp"

#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <atomic>

using json = nlohmann::json;

namespace ailee {

// Implementation class using PIMPL idiom
class AILEEWebServer::Impl {
public:
    Impl(const WebServerConfig& config) : config_(config) {
        server_ = std::make_unique<httplib::Server>();
        setupRoutes();
    }

    ~Impl() {
        stop();
    }

    bool start() {
        if (running_) return false;
        
        running_ = true;
        server_thread_ = std::thread([this]() {
            std::cout << "[WebServer] Starting on " << config_.host 
                      << ":" << config_.port << std::endl;
            
            if (config_.enable_ssl && !config_.ssl_cert_path.empty() 
                && !config_.ssl_key_path.empty()) {
                // ERROR: SSL was requested but is not fully implemented.
                // Refusing to silently downgrade to plain HTTP — fail-closed.
                std::cerr << "[WebServer] ERROR: SSL/TLS is configured but not fully implemented. "
                             "Refusing to start in plain HTTP mode. "
                             "Disable enable_ssl or provide a complete SSL implementation." << std::endl;
                running_ = false;
                return;
            } else {
                server_->listen(config_.host.c_str(), config_.port);
            }
            running_ = false;
        });
        
        return true;
    }

    void stop() {
        if (server_) {
            server_->stop();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        running_ = false;
    }

    bool isRunning() const {
        return running_;
    }

    void setNodeStatusCallback(std::function<NodeStatus()> callback) {
        status_callback_ = callback;
    }

    void setOrchestratorRef(Orchestrator* orch) {
        orchestrator_ = orch;
    }

    void setLedgerRef(Ledger* ledger) {
        ledger_ = ledger;
    }

    void setBlockProducerRef(l2::BlockProducer* producer) {
        block_producer_ = producer;
    }

    void setMempoolRef(l2::Mempool* mempool) {
        mempool_ = mempool;
    }

    void setReplayTickCallback(std::function<std::string(uint64_t)> callback) {
        replay_tick_callback_ = callback;
    }

    void setFederationViewCallback(std::function<std::string()> callback) {
        federation_view_callback_ = callback;
    }

    void setSyncEventsCallback(std::function<std::string()> callback) {
        sync_events_callback_ = callback;
    }

    void setSyncClockCallback(std::function<std::string()> callback) {
        sync_clock_callback_ = callback;
    }

    void setLatestReplayTickCallback(std::function<std::string()> callback) {
        latest_replay_tick_callback_ = callback;
    }

    void setMeshEnvelopesCallback(std::function<std::string()> callback) {
        mesh_envelopes_callback_ = callback;
    }

    bool isReplayModeEnabled() const {
        return replay_mode_.load();
    }

    bool isFederationModeEnabled() const {
        return federation_mode_.load();
    }

private:
    void setupRoutes() {
        // Combined pre-routing handler: CORS headers first, then API key auth.
        // Both are applied in one handler because httplib only keeps the last
        // set_pre_routing_handler registration; registering two handlers would
        // silently discard the first (CORS), leaving the browser unable to
        // reach the API when authentication is also enabled.
        server_->set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
            // Apply CORS headers on every response when CORS is enabled.
            if (config_.enable_cors) {
                // SECURITY NOTE: Access-Control-Allow-Origin is set to "*" (wildcard).
                // In production, restrict this to specific trusted origins by setting
                // cors_origins in the WebServerConfig.
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-API-Key");

                if (req.method == "OPTIONS") {
                    res.status = 200;
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            // Enforce API key on /api/* routes when a key is configured.
            if (!config_.api_key.empty() && req.path.find("/api/") == 0) {
                auto api_key = req.get_header_value("X-API-Key");
                if (api_key != config_.api_key) {
                    res.status = 401;
                    json error_response;
error_response = {
                        {"error", "Unauthorized"},
                        {"message", "Invalid or missing API key"}
                    };
                    res.set_content(error_response.dump(), "application/json");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            return httplib::Server::HandlerResponse::Unhandled;
        });

        // Serve the web dashboard
        server_->Get("/", [](const httplib::Request&, httplib::Response& res) {
            std::ifstream file("web/index.html");
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.set_content(buffer.str(), "text/html");
            } else {
                json response;
                response = {
                    {"name", "AILEE Protocol Core API"},
                    {"version", "1.0.0"},
                    {"description", "REST API for AILEE Bitcoin Layer-2 Protocol"},
                    {"endpoints", {
                        {"/api/status", "Get node status and health"},
                        {"/api/metrics", "Get system metrics"},
                        {"/api/l2/state", "Get Layer-2 state information"},
                        {"/api/orchestration/tasks", "Get orchestration task list"},
                        {"/api/anchors/latest", "Get latest Bitcoin anchor"},
                        {"/api/health", "Health check endpoint"},
                        {"/api/replay/tick/:index", "Get deterministic replay tick by index"},
                        {"/api/federation/view", "Get deterministic federation view"}
                    }},
                    {"documentation", "https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin"}
                };
                res.set_content(response.dump(), "application/json");
            }
        });

        // Health check endpoint
        server_->Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            json response;
                response = {
                {"status", "healthy"},
                {"timestamp", static_cast<double>(ms)}
            };
            res.set_content(response.dump(), "application/json");
        });

        // Node status endpoint
        server_->Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
            if (status_callback_) {
                try {
                    NodeStatus status = status_callback_();
                    json response;
                response = {
                        {"running", status.running},
                        {"version", status.version},
                        {"uptime_seconds", json::number_unsigned(status.uptime_seconds)},
                        {"network", status.network},
                        {"statistics", {
                            {"total_transactions", json::number_unsigned(status.total_transactions)},
                            {"total_blocks", json::number_unsigned(status.total_blocks)},
                            {"current_tps", status.current_tps},
                            {"pending_tasks", json::number_unsigned(status.pending_tasks)}
                        }},
                        {"last_anchor_hash", status.last_anchor_hash}
                    };
                    res.set_content(response.dump(), "application/json");
                } catch (const std::exception& e) {
                    res.status = 500;
                    json error;
                error = {{"error", e.what()}};
                    res.set_content(error.dump(), "application/json");
                }
            } else {
                json response;
                response = {
                    {"status", "running"},
                    {"message", "Status callback not configured"}
                };
                res.set_content(response.dump(), "application/json");
            }
        });

        // Metrics endpoint
        server_->Get("/api/metrics", [this](const httplib::Request&, httplib::Response& res) {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            json metrics;
metrics = {
                {"timestamp", static_cast<double>(ms)},
                {"node", {
                    {"type", "AILEE-Core"},
                    {"layer", "Bitcoin Layer-2"}
                }}
            };
            
            if (status_callback_) {
                try {
                    NodeStatus status = status_callback_();
                    metrics["performance"] = {
                        {"current_tps", status.current_tps},
                        {"pending_tasks", json::number_unsigned(status.pending_tasks)}
                    };
                } catch (...) {
                    // Ignore errors for metrics
                }
            }
            
            res.set_content(metrics.dump(), "application/json");
        });

        // Layer-2 state endpoint - now with real block data
        server_->Get("/api/l2/state", [this](const httplib::Request&, httplib::Response& res) {
            json state;
state = {
                {"layer", "Layer-2"},
                {"protocol", "AILEE-Core"},
                {"description", "Bitcoin-anchored Layer-2 state"}
            };
            
            if (ledger_) {
                state["ledger"] = {
                    {"status", "active"},
                    {"type", "federated"}
                };
            }
            
            // Add block producer state if available
            if (block_producer_) {
                auto blockState = block_producer_->getState();
                state["block_height"] = json::number_unsigned(blockState.blockHeight);
                state["total_transactions"] = json::number_unsigned(blockState.totalTransactions);
                state["last_anchor_height"] = json::number_unsigned(blockState.lastAnchorHeight);
            } else {
                // Fallback if block producer not initialized yet
                state["block_height"] = 0;
                state["total_transactions"] = 0;
                state["last_anchor_height"] = 0;
            }
            
            res.set_content(state.dump(), "application/json");
        });

        // Orchestration tasks endpoint
        server_->Get("/api/orchestration/tasks", [this](const httplib::Request&, httplib::Response& res) {
            json tasks;
tasks = {
                {"tasks", json::array({})},
                {"total", 0}
            };
            
            if (orchestrator_) {
                tasks["status"] = "available";
            } else {
                tasks["status"] = "not_configured";
            }
            
            res.set_content(tasks.dump(), "application/json");
        });

        // Latest anchor endpoint
        server_->Get("/api/anchors/latest", [this](const httplib::Request&, httplib::Response& res) {
            json anchor;
anchor = {
                {"message", "Bitcoin anchoring is active"}
            };
            
            if (status_callback_) {
                try {
                    NodeStatus status = status_callback_();
                    anchor["last_anchor_hash"] = status.last_anchor_hash;
                } catch (...) {
                    anchor["last_anchor_hash"] = "N/A";
                }
            }
            
            res.set_content(anchor.dump(), "application/json");
        });

        // Submit task endpoint (POST)
        server_->Post("/api/orchestration/submit", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json request_body = json::parse(req.body);
                
                if (!request_body.contains("task_type") || !request_body.contains("task_data")) {
                    res.status = 400;
                    json error;
                error = {
                        {"error", "Invalid request"},
                        {"message", "task_type and task_data are required"}
                    };
                    res.set_content(error.dump(), "application/json");
                    return;
                }
                
                static std::atomic<uint64_t> task_counter{0};
                auto now = std::chrono::system_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                uint64_t counter = task_counter.fetch_add(1);
                
                std::ostringstream task_id_stream;
                task_id_stream << "task_" << ms << "_" << counter;
                
                json response;
                response = {
                    {"status", "accepted"},
                    {"task_id", task_id_stream.str()},
                    {"message", "Task submitted successfully"}
                };
                
                res.status = 202;
                res.set_content(response.dump(), "application/json");
                
            } catch (const std::exception& e) {
                res.status = 400;
                json error;
                error = {
                    {"error", "Invalid request"},
                    {"message", e.what()}
                };
                res.set_content(error.dump(), "application/json");
            }
        });

        // Deterministic Replay Tick endpoint
        server_->Get(R"(/api/replay/tick/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            if (!replay_tick_callback_) {
                res.status = 501;
                json error;
                error = {
                    {"error", "Not Implemented"},
                    {"message", "Replay tick callback not configured"}
                };
                res.set_content(error.dump(), "application/json");
                return;
            }

            try {
                uint64_t index = std::stoull(req.matches[1]);
                std::string tick_json = replay_tick_callback_(index);
                if (tick_json.empty()) {
                    res.status = 404;
                    json error;
                error = {
                        {"error", "Not Found"},
                        {"message", "Tick index out of bounds"}
                    };
                    res.set_content(error.dump(), "application/json");
                } else {
                    res.set_content(tick_json, "application/json");
                }
            } catch (...) {
                res.status = 400;
                json error;
                error = {
                    {"error", "Invalid Request"},
                    {"message", "Invalid tick index format"}
                };
                res.set_content(error.dump(), "application/json");
            }
        });

        // Deterministic Federation View endpoint
        server_->Get("/api/federation/view", [this](const httplib::Request&, httplib::Response& res) {
            if (!federation_view_callback_) {
                res.status = 501;
                json error;
                error = {
                    {"error", "Not Implemented"},
                    {"message", "Federation view callback not configured"}
                };
                res.set_content(error.dump(), "application/json");
                return;
            }

            if (!federation_mode_.load()) {
                res.status = 200;
                json response;
                response = {
                    {"status", "disabled"},
                    {"message", "Federation mode is OFF"}
                };
                res.set_content(response.dump(), "application/json");
                return;
            }

            std::string view_json = federation_view_callback_();
            res.set_content(view_json, "application/json");
        });

        // V17 Telemetry Endpoints
        
        server_->Get("/api/sync/events", [this](const httplib::Request&, httplib::Response& res) {
            if (!sync_events_callback_) {
                res.status = 501;
                json error;
                error = {
                    {"error", "Not Implemented"},
                    {"message", "Sync events callback not configured"}
                };
                res.set_content(error.dump(), "application/json");
                return;
            }
            res.set_content(sync_events_callback_(), "application/json");
        });

        server_->Get("/api/sync/clock", [this](const httplib::Request&, httplib::Response& res) {
            if (!sync_clock_callback_) {
                res.status = 501;
                json error;
                error = {
                    {"error", "Not Implemented"},
                    {"message", "Sync clock callback not configured"}
                };
                res.set_content(error.dump(), "application/json");
                return;
            }
            res.set_content(sync_clock_callback_(), "application/json");
        });

        server_->Get("/api/replay/tick", [this](const httplib::Request&, httplib::Response& res) {
            if (!latest_replay_tick_callback_) {
                res.status = 501;
                json error;
                error = {
                    {"error", "Not Implemented"},
                    {"message", "Latest replay tick callback not configured"}
                };
                res.set_content(error.dump(), "application/json");
                return;
            }
            res.set_content(latest_replay_tick_callback_(), "application/json");
        });

        // Heartbeat (simple liveness flag)
        server_->Get("/api/heartbeat", [this](const httplib::Request&, httplib::Response& res) {
            json response;
                response = {
                {"status", heartbeat_ok_ ? "ok" : "degraded"}
            };
            res.set_content(response.dump(), "application/json");
        });

        // Eject (no-op placeholder)
        server_->Post("/api/eject", [this](const httplib::Request&, httplib::Response& res) {
            json response;
                response = {
                {"status", "accepted"},
                {"message", "Eject requested (no-op in testnet)"}
            };
            res.status = 202;
            res.set_content(response.dump(), "application/json");
        });

        // Reject (no-op placeholder)
        server_->Post("/api/reject", [this](const httplib::Request&, httplib::Response& res) {
            json response;
                response = {
                {"status", "accepted"},
                {"message", "Reject requested (no-op in testnet)"}
            };
            res.status = 202;
            res.set_content(response.dump(), "application/json");
        });

        // Federation mode toggle
        server_->Post("/api/federation/mode", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                bool enable = body.value("enable", false);
                federation_mode_.store(enable);
                json response;
                response = {
                    {"status", "ok"},
                    {"federation_mode", enable ? "ON" : "OFF"}
                };
                res.set_content(response.dump(), "application/json");
            } catch (...) {
                res.status = 400;
                json error;
                error = {{"error", "Invalid request"}};
                res.set_content(error.dump(), "application/json");
            }
        });

        // Replay mode toggle
        server_->Post("/api/replay/mode", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                bool enable = body.value("enable", false);
                replay_mode_.store(enable);
                json response;
                response = {
                    {"status", "ok"},
                    {"replay_mode", enable ? "ON" : "OFF"}
                };
                res.set_content(response.dump(), "application/json");
            } catch (...) {
                res.status = 400;
                json error;
                error = {{"error", "Invalid request"}};
                res.set_content(error.dump(), "application/json");
            }
        });

        // Mesh envelopes for Deterministic Mesh panel
        server_->Get("/api/mesh/envelopes", [this](const httplib::Request&, httplib::Response& res) {
            if (!mesh_envelopes_callback_) {
                res.status = 501;
                json error;
                error = {
                    {"error", "Not Implemented"},
                    {"message", "Mesh envelopes callback not configured"}
                };
                res.set_content(error.dump(), "application/json");
                return;
            }

            if (!federation_mode_.load()) {
                res.status = 200;
                json response;
                response = {
                    {"status", "disabled"},
                    {"message", "Federation mode is OFF"}
                };
                res.set_content(response.dump(), "application/json");
                return;
            }

            res.set_content(mesh_envelopes_callback_(), "application/json");
        });

        // Refresh All (just a convenience ping)
        server_->Post("/api/refresh/all", [this](const httplib::Request&, httplib::Response& res) {
            json response;
                response = {
                {"status", "ok"},
                {"message", "Refresh requested"}
            };
            res.set_content(response.dump(), "application/json");
        });

        // Transaction submission endpoint (POST)
        server_->Post("/api/transactions/submit", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!mempool_) {
                    res.status = 503;
                    json error;
                error = {
                        {"error", "Service Unavailable"},
                        {"message", "Mempool not initialized"}
                    };
                    res.set_content(error.dump(), "application/json");
                    return;
                }
                
                json request_body = json::parse(req.body);
                
                // Validate required fields
                if (!request_body.contains("from_address") || 
                    !request_body.contains("to_address") || 
                    !request_body.contains("amount") ||
                    !request_body.contains("tx_hash")) {
                    res.status = 400;
                    json error;
                error = {
                        {"error", "Invalid request"},
                        {"message", "from_address, to_address, amount, and tx_hash are required"}
                    };
                    res.set_content(error.dump(), "application/json");
                    return;
                }
                
                // Create transaction
                l2::Transaction tx;
                tx.txHash = request_body["tx_hash"].get<std::string>();
                tx.fromAddress = request_body["from_address"].get<std::string>();
                tx.toAddress = request_body["to_address"].get<std::string>();
                tx.amount = request_body["amount"].get<std::uint64_t>();
                tx.data = request_body.value("data", "");
                tx.signature = request_body.value("signature", "");
                tx.publicKey = request_body.value("public_key", tx.fromAddress);
                tx.status = "pending";
                tx.blockHeight = 0;
                
                // Get timestamp
                auto now = std::chrono::system_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                tx.timestampMs = static_cast<std::uint64_t>(ms);
                
                // Add to mempool (returns false if duplicate)
                if (!mempool_->addTransaction(tx)) {
                    res.status = 409;
                    json error;
                error = {
                        {"error", "Conflict"},
                        {"message", "Transaction with this hash already exists in mempool"}
                    };
                    res.set_content(error.dump(), "application/json");
                    return;
                }
                
                std::cout << "[WebServer] Transaction added to mempool: " << tx.txHash.substr(0, 16) 
                          << "... from " << tx.fromAddress << " to " << tx.toAddress 
                          << " amount " << tx.amount << std::endl;
                
                json response;
                response = {
                    {"status", "accepted"},
                    {"tx_hash", tx.txHash},
                    {"message", "Transaction submitted to mempool"}
                };
                
                res.status = 202;
                res.set_content(response.dump(), "application/json");
                
            } catch (const std::exception& e) {
                res.status = 400;
                json error;
                error = {
                    {"error", "Invalid request"},
                    {"message", e.what()}
                };
                res.set_content(error.dump(), "application/json");
            }
        });

        // 404 handler
        server_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
            json error;
                error = {
                {"error", "Not Found"},
                {"message", "The requested endpoint does not exist"},
                {"status_code", res.status}
            };
            res.set_content(error.dump(), "application/json");
        });
    }

    WebServerConfig config_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    
    std::function<NodeStatus()> status_callback_;
    Orchestrator* orchestrator_ = nullptr;
    Ledger* ledger_ = nullptr;
    l2::BlockProducer* block_producer_ = nullptr;
    l2::Mempool* mempool_ = nullptr;
    std::function<std::string(uint64_t)> replay_tick_callback_;
    std::function<std::string()> federation_view_callback_;
    std::function<std::string()> sync_events_callback_;
    std::function<std::string()> sync_clock_callback_;
    std::function<std::string()> latest_replay_tick_callback_;
    std::function<std::string()> mesh_envelopes_callback_;
    std::atomic<bool> heartbeat_ok_{true};
    std::atomic<bool> federation_mode_{false};
    std::atomic<bool> replay_mode_{false};
};

// Public API implementation
AILEEWebServer::AILEEWebServer(const WebServerConfig& config) 
    : pImpl(std::make_unique<Impl>(config)), config_(config) {
}

AILEEWebServer::~AILEEWebServer() = default;

bool AILEEWebServer::start() {
    return pImpl->start();
}

void AILEEWebServer::stop() {
    pImpl->stop();
}

bool AILEEWebServer::isRunning() const {
    return pImpl->isRunning();
}

bool AILEEWebServer::isReplayModeEnabled() const {
    return pImpl->isReplayModeEnabled();
}

bool AILEEWebServer::isFederationModeEnabled() const {
    return pImpl->isFederationModeEnabled();
}

void AILEEWebServer::setNodeStatusCallback(std::function<NodeStatus()> callback) {
    pImpl->setNodeStatusCallback(callback);
}

void AILEEWebServer::setOrchestratorRef(Orchestrator* orch) {
    pImpl->setOrchestratorRef(orch);
}

void AILEEWebServer::setLedgerRef(Ledger* ledger) {
    pImpl->setLedgerRef(ledger);
}

void AILEEWebServer::setBlockProducerRef(l2::BlockProducer* producer) {
    pImpl->setBlockProducerRef(producer);
}

void AILEEWebServer::setMempoolRef(l2::Mempool* mempool) {
    pImpl->setMempoolRef(mempool);
}

void AILEEWebServer::setReplayTickCallback(std::function<std::string(uint64_t)> callback) {
    pImpl->setReplayTickCallback(callback);
}

void AILEEWebServer::setFederationViewCallback(std::function<std::string()> callback) {
    pImpl->setFederationViewCallback(callback);
}

void AILEEWebServer::setSyncEventsCallback(std::function<std::string()> callback) {
    pImpl->setSyncEventsCallback(callback);
}

void AILEEWebServer::setSyncClockCallback(std::function<std::string()> callback) {
    pImpl->setSyncClockCallback(callback);
}

void AILEEWebServer::setLatestReplayTickCallback(std::function<std::string()> callback) {
    pImpl->setLatestReplayTickCallback(callback);
}

void AILEEWebServer::setMeshEnvelopesCallback(std::function<std::string()> callback) {
    pImpl->setMeshEnvelopesCallback(callback);
}

} // namespace ailee
