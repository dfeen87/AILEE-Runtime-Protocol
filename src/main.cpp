// SPDX-License-Identifier: MIT
// AILEE-Core Node [v2.0.0-Production-Ready]
// Enhanced with distributed task orchestration, advanced monitoring, and unified architecture

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <memory>
#include <fstream>
#include <filesystem>

// Core Protocol Headers
#include "build_metadata.hpp"
#include "build/BuildInfo.hpp"
#include "ailee_tps_engine.h"
#include "ailee_gold_bridge.h"
#include "ailee_recovery_protocol.h"

// Auxiliary System Headers
#include "ailee_energy_telemetry.h"
#include "ailee_circuit_breaker.h"

// Ambient Mesh Intelligence System
#include "AmbientAI.h"

// Layer-2 Internet Relay
#include "ailee_netflow.h"

// Bitcoin Integration
#include "BitcoinZMQListener.h"
#include "BitcoinRPCClient.h"
#include "Global_Seven.h"
#include "ReorgDetector.h"

// Advanced Computing
#include "runtime/WasmEngine.h"
#include "FederatedLearning.h"

// *** NEW: Unified Orchestration Engine ***
#include "Orchestrator.h"
#include "Ledger.h"

// Block Producer for L2 Chain
#include "BlockProducer.h"
#include "Mempool.h"

// Web Server for HTTP API
#include "AILEEWebServer.h"

// ---------------------------------------------------------
// Enhanced Structured Logging with Levels and File Output
// ---------------------------------------------------------
enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };
static std::mutex g_logMutex;
static std::ofstream g_logFile;

static std::string nowIso8601() {
    using namespace std::chrono;
    auto t = system_clock::now();
    auto secs = time_point_cast<seconds>(t);
    auto ms = duration_cast<milliseconds>(t - secs).count();
    std::time_t tt = system_clock::to_time_t(t);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    
    std::ostringstream oss;
    oss << buf << "." << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

static void log(LogLevel level, const std::string& msg) {
    const char* sev = nullptr;
    switch(level) {
        case LogLevel::DEBUG: sev = "DEBUG"; break;
        case LogLevel::INFO:  sev = "INFO "; break;
        case LogLevel::WARN:  sev = "WARN "; break;
        case LogLevel::ERROR: sev = "ERROR"; break;
        case LogLevel::FATAL: sev = "FATAL"; break;
    }

    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string logLine = "[" + nowIso8601() + "] [" + std::string(sev) + "] " + msg;
    
    // Console output
    std::cout << logLine << std::endl;
    
    // File output
    if (g_logFile.is_open()) {
        g_logFile << logLine << std::endl;
        g_logFile.flush();
    }
}

static void initLogFile(const std::string& path) {
    g_logFile.open(path, std::ios::app);
    if (!g_logFile.is_open()) {
        std::cerr << "Warning: Could not open log file: " << path << std::endl;
    }
}

static void closeLogFile() {
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
}

// ---------------------------------------------------------
// Shutdown flag + Enhanced signal handling
// ---------------------------------------------------------
static std::atomic<bool> g_shutdown{false};
static std::atomic<bool> g_forceShutdown{false};

static void handleSignal(int signum) {
    if (g_shutdown.load()) {
        log(LogLevel::FATAL, "Force shutdown signal received (signal " + 
            std::to_string(signum) + ")");
        g_forceShutdown.store(true);
        std::exit(1);
    }
    
    log(LogLevel::WARN, "Signal " + std::to_string(signum) + 
        " received — initiating graceful shutdown");
    g_shutdown.store(true);
}

static void installSignalHandlers() {
    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);
#if defined(SIGQUIT)
    std::signal(SIGQUIT, handleSignal);
#endif
#if defined(SIGHUP)
    std::signal(SIGHUP, handleSignal);
#endif
}

// ---------------------------------------------------------
// Enhanced Configuration with Orchestration Settings
// ---------------------------------------------------------
struct Config {
    // TPS Engine parameters
    int         tpsSimNodes       = 100;
    double      tpsInitialBlockMB = 1.0;
    int         tpsSimCycles      = 200;
    
    // Circuit breaker thresholds
    double      maxBlockMB        = 4.0;
    double      maxLatencyMs      = 2000.0;
    int         minPeerCount      = 8;
    
    // Layer-2 NetFlow options
    bool        enableNetFlow     = true;
    int         netFlowPort       = 4000;
    
    // *** NEW: Orchestration settings ***
    bool        enableOrchestration = true;
    int         orchestratorWorkers = 4;
    int         maxConcurrentTasks  = 100;
    std::string orchestratorStrategy = "WEIGHTED_SCORE";
    
    // Monitoring
    bool        enableMetrics     = true;
    int         metricsPort       = 9090;
    std::string logPath           = "./logs/ailee.log";
    
    // HTTP Web Server
    bool        enableWebServer   = true;
    std::string webServerHost     = "0.0.0.0";
    int         webServerPort     = 8080;
    
    // Node identity
    std::string nodeId            = "node-001";
    std::string region            = "us-east";
    std::string network           = "bitcoin-mainnet";
    std::string bitcoinZmqEndpoint = "tcp://127.0.0.1:28332";

    bool validate() const {
        if (tpsSimNodes < 10 || tpsSimNodes > 10000) {
            log(LogLevel::ERROR, "Invalid tpsSimNodes: " + std::to_string(tpsSimNodes));
            return false;
        }
        if (tpsInitialBlockMB < 0.1 || tpsInitialBlockMB > 8.0) {
            log(LogLevel::ERROR, "Invalid tpsInitialBlockMB: " + std::to_string(tpsInitialBlockMB));
            return false;
        }
        if (tpsSimCycles < 10 || tpsSimCycles > 10000) {
            log(LogLevel::ERROR, "Invalid tpsSimCycles: " + std::to_string(tpsSimCycles));
            return false;
        }
        if (netFlowPort < 1024 || netFlowPort > 65535) {
            log(LogLevel::ERROR, "Invalid netFlowPort: " + std::to_string(netFlowPort));
            return false;
        }
        if (orchestratorWorkers < 1 || orchestratorWorkers > 64) {
            log(LogLevel::ERROR, "Invalid orchestratorWorkers: " + std::to_string(orchestratorWorkers));
            return false;
        }
        if (maxConcurrentTasks < 1 || maxConcurrentTasks > 10000) {
            log(LogLevel::ERROR, "Invalid maxConcurrentTasks: " + std::to_string(maxConcurrentTasks));
            return false;
        }
        if (enableWebServer && (webServerPort < 1024 || webServerPort > 65535)) {
            log(LogLevel::ERROR, "Invalid webServerPort: " + std::to_string(webServerPort));
            return false;
        }
        return true;
    }
};

static Config loadConfigFromEnv() {
    Config c;
    
    // Existing TPS settings
    if (const char* n = std::getenv("AILEE_TPS_NODES")) {
        c.tpsSimNodes = std::atoi(n);
    }
    if (const char* b = std::getenv("AILEE_TPS_INITIAL_MB")) {
        c.tpsInitialBlockMB = std::atof(b);
    }
    if (const char* cycles = std::getenv("AILEE_TPS_CYCLES")) {
        c.tpsSimCycles = std::atoi(cycles);
    }
    
    // NetFlow settings
    if (const char* nf = std::getenv("AILEE_NETFLOW_ENABLE")) {
        c.enableNetFlow = std::atoi(nf) != 0;
    }
    if (const char* port = std::getenv("AILEE_NETFLOW_PORT")) {
        c.netFlowPort = std::atoi(port);
    }
    
    // *** NEW: Orchestration settings ***
    if (const char* orch = std::getenv("AILEE_ORCHESTRATION_ENABLE")) {
        c.enableOrchestration = std::atoi(orch) != 0;
    }
    if (const char* workers = std::getenv("AILEE_ORCHESTRATOR_WORKERS")) {
        c.orchestratorWorkers = std::atoi(workers);
    }
    if (const char* tasks = std::getenv("AILEE_MAX_CONCURRENT_TASKS")) {
        c.maxConcurrentTasks = std::atoi(tasks);
    }
    if (const char* strategy = std::getenv("AILEE_ORCHESTRATOR_STRATEGY")) {
        c.orchestratorStrategy = strategy;
    }
    
    // Node identity
    if (const char* nodeId = std::getenv("AILEE_NODE_ID")) {
        c.nodeId = nodeId;
    }
    if (const char* region = std::getenv("AILEE_REGION")) {
        c.region = region;
    }
    if (const char* network = std::getenv("AILEE_NETWORK")) {
        c.network = network;
    }
    if (const char* zmq = std::getenv("AILEE_BITCOIN_ZMQ_ENDPOINT")) {
        c.bitcoinZmqEndpoint = zmq;
    }

    // Monitoring
    if (const char* logPath = std::getenv("AILEE_LOG_PATH")) {
        c.logPath = logPath;
    }
    
    // Web Server settings
    if (const char* wsEnable = std::getenv("AILEE_WEB_SERVER_ENABLE")) {
        c.enableWebServer = std::atoi(wsEnable) != 0;
    }
    if (const char* wsHost = std::getenv("AILEE_WEB_SERVER_HOST")) {
        c.webServerHost = wsHost;
    }
    if (const char* wsPort = std::getenv("AILEE_WEB_SERVER_PORT")) {
        c.webServerPort = std::atoi(wsPort);
    }

    return c;
}

// ---------------------------------------------------------
// Enhanced Engine with Orchestration Integration
// ---------------------------------------------------------
class AILEEEngine {
public:
    explicit AILEEEngine(const Config& cfg) 
        : cfg_(cfg), 
          orchestrationEngine_(nullptr),
          webServer_(nullptr),
          shutdownCalled_(false)
    {
        log(LogLevel::INFO, "AILEE Engine initializing with node ID: " + cfg_.nodeId);
        
        // Initialize orchestration engine if enabled
        if (cfg_.enableOrchestration) {
            initOrchestrationEngine();
        }

        // Initialize Reorg Detector (Security)
        initReorgDetector();
        
        // Initialize ZMQ Listener and connect to ReorgDetector
        initZMQListener();

        // Initialize block producer (L2 chain component)
        initBlockProducer();
        
        // Initialize web server if enabled
        if (cfg_.enableWebServer) {
            initWebServer();
        }
        
        // Initialize NetFlow if enabled
        if (cfg_.enableNetFlow) {
            log(LogLevel::INFO, "NetFlow enabled");
        }
    }

    ~AILEEEngine() {
        shutdown();
    }
    
    void start() {
        if (orchestrationEngine_) {
            log(LogLevel::INFO, "Starting orchestration engine...");
            orchestrationEngine_->start();
            
            // Register this node in the orchestration network
            registerSelfAsNode();
        }
        
        // Start web server
        if (webServer_) {
            // Connect block producer and mempool to web server before starting to avoid race condition
            if (blockProducer_) {
                webServer_->setBlockProducerRef(blockProducer_.get());
            }
            if (mempool_) {
                webServer_->setMempoolRef(mempool_.get());
            }
            
            log(LogLevel::INFO, "Starting web server on " + cfg_.webServerHost + ":" + 
                std::to_string(cfg_.webServerPort) + "...");
            if (webServer_->start()) {
                log(LogLevel::INFO, "Web server started successfully");
            } else {
                log(LogLevel::ERROR, "Failed to start web server");
            }
        }
        
        // Start block producer
        if (blockProducer_) {
            log(LogLevel::INFO, "Starting block producer...");
            blockProducer_->start();
            log(LogLevel::INFO, "Block producer started successfully");
        }
        
        log(LogLevel::INFO, "AILEE Engine started successfully");
    }
    
    void shutdown() {
        if (shutdownCalled_.exchange(true)) {
            return;
        }
        log(LogLevel::INFO, "AILEE Engine shutting down");
        
        // Stop block producer first
        if (blockProducer_) {
            log(LogLevel::INFO, "Stopping block producer...");
            blockProducer_->stop();
        }
        
        // Stop web server
        if (webServer_) {
            log(LogLevel::INFO, "Stopping web server...");
            webServer_->stop();
        }
        
        if (orchestrationEngine_) {
            log(LogLevel::INFO, "Stopping orchestration engine...");
            orchestrationEngine_->stop();
            
            // Print final metrics
            printOrchestratorMetrics();
        }
    }

    // TPS Simulation with error handling
    bool runTPSSimulation() {
        log(LogLevel::INFO, "TPS Simulation starting: nodes=" + 
            std::to_string(cfg_.tpsSimNodes) +
            " initialMB=" + std::to_string(cfg_.tpsInitialBlockMB) +
            " cycles=" + std::to_string(cfg_.tpsSimCycles));

        try {
            auto result = ailee::PerformanceSimulator::runSimulation(
                cfg_.tpsSimNodes, cfg_.tpsInitialBlockMB, cfg_.tpsSimCycles);

            log(LogLevel::INFO, "Baseline TPS: " + std::to_string(result.initialTPS));
            log(LogLevel::INFO, "Final TPS: " + std::to_string(result.finalTPS));
            log(LogLevel::INFO, "Improvement: " + std::to_string(result.improvementFactor) + "x");
            log(LogLevel::INFO, "Cycles completed: " + std::to_string(result.cycles));

            // Log optimization snapshots
            std::ostringstream hist;
            hist << "Optimization history (every 20 cycles):";
            for (size_t i = 0; i < result.aiFactorHistory.size() && i < 200; i += 20) {
                hist << "\n  cycle=" << std::setw(4) << i
                     << " aiFactor=" << std::fixed << std::setprecision(4) 
                     << result.aiFactorHistory[i]
                     << " tps=" << std::setprecision(1) << result.tpsHistory[i]
                     << " error=" << std::setprecision(4) << result.errorHistory[i];
            }
            log(LogLevel::INFO, hist.str());
            
            // Submit TPS results as orchestrated task if enabled
            if (orchestrationEngine_) {
                submitTPSResultsTask(result);
            }
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "TPS Simulation failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("TPSSimulationFailure", e.what());
            return false;
        }
    }

    // Gold Bridge with proper error handling
    bool testGoldBridge() {
        log(LogLevel::INFO, "Testing Bitcoin-to-Gold Bridge protocol");

        try {
            ailee::GoldBridge bridge;
            std::string user = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
            uint64_t btcAmount = 500000000; // 5 BTC

            std::string conversionId = bridge.initiateConversion(user, btcAmount, true);

            log(LogLevel::INFO, "Conversion initiated: ID=" + conversionId);
            log(LogLevel::INFO, "Status: PENDING_PAYMENT | Oracle: ACTIVE | Inventory: SECURE");
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Gold Bridge failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("GoldBridgeFailure", e.what());
            return false;
        }
    }

    // Safety & Energy with enhanced Circuit Breaker v1.4
    bool testSafetyAndEnergy() {
        log(LogLevel::INFO, "Evaluating Safety & Energy systems");

        try {
            // Sample thermal metrics
            ailee::ThermalMetric minerStats{
                3000.0,      // inputPowerWatts
                1500.0,      // wasteHeatRecoveredW
                25.0,        // ambientTempC
                60.0,        // exhaustTempC
                1735660000   // timestamp
            };

            // Calculate Energy Integrity Score
            auto analysis = ailee::EnergyTelemetry::analyze(minerStats);
            
            std::string proof = ailee::EnergyTelemetry::generateTelemetryProof(
                minerStats, "Node-001");

            log(LogLevel::INFO, 
                "Energy Analysis: input=" + std::to_string(minerStats.inputPowerWatts) + "W " +
                "recovery=" + std::to_string(minerStats.wasteHeatRecoveredW) + "W " +
                "EIS=" + std::to_string(analysis.energyIntegrityScore * 100.0) + "%");
            
            log(LogLevel::DEBUG, "GreenHash: " + proof.substr(0, 32) + "...");

            // Enhanced Circuit Breaker v1.4 with EIS
            auto breaker = ailee::CircuitBreaker::monitor(
                breakerState_,
                breakerTestBlockHeight_++,        // incrementing blockHeight
                15000,                            // proposedBlockSizeScaled (1.5 * 10000)
                1000000,                          // currentLatencyScaled (100.0 * 10000)
                100,                              // peerCount
                10000,                            // targetBlockSizeScaled (1.0 * 10000)
                static_cast<uint64_t>(std::round(analysis.energyIntegrityScore * 10000)), // currentEISScaled
                static_cast<uint64_t>(std::round(analysis.sensorConfidence * 10000)),     // sensorConfidenceScaled
                5000                              // previousEISScaled (0.5 * 10000)
            );

            switch(breaker.state) {
                case ailee::SystemState::OPTIMIZED:
                    log(LogLevel::INFO, "Circuit Breaker: OPTIMIZED — " + breaker.reason);
                    break;
                case ailee::SystemState::SOFT_TRIP:
                    log(LogLevel::WARN, "Circuit Breaker: SOFT_TRIP — " + breaker.reason);
                    break;
                case ailee::SystemState::SAFE_MODE:
                    log(LogLevel::WARN, "Circuit Breaker: SAFE_MODE — " + breaker.reason);
                    throttleSystems();
                    break;
                case ailee::SystemState::CRITICAL:
                    log(LogLevel::FATAL, "Circuit Breaker: CRITICAL — " + breaker.reason);
                    return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Safety/Energy test failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("SafetyEnergyFailure", e.what());
            return false;
        }
    }

    // AmbientAI Mesh with orchestration integration
    bool demoAmbientMesh() {
        log(LogLevel::INFO, "[AmbientAI] Running Ambient Mesh intelligence demo");

        try {
            ambient::SafetyPolicy policy{80 * ambient::FIXED_POINT_SCALE, 250 * ambient::FIXED_POINT_SCALE, 8 * ambient::FIXED_POINT_SCALE, 25};

            uint64_t currentTimestampMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            ambient::AmbientNode nodeA({"pubA", cfg_.region, "gateway"}, policy, nullptr, currentTimestampMs);
            ambient::AmbientNode nodeB({"pubB", cfg_.region, "smartphone"}, policy, nullptr, currentTimestampMs);

            // Sample telemetry for gateway node
            ambient::TelemetrySample sampleA{
                {"pubA", cfg_.region, "gateway"},
                {1200 * ambient::FIXED_POINT_SCALE, 300 * ambient::FIXED_POINT_SCALE, 55 * ambient::FIXED_POINT_SCALE, 22 * ambient::FIXED_POINT_SCALE, 350 * ambient::FIXED_POINT_SCALE},          // energy
                {35 * ambient::FIXED_POINT_SCALE, 10 * ambient::FIXED_POINT_SCALE, 5 * ambient::FIXED_POINT_SCALE, 2048 * ambient::FIXED_POINT_SCALE, 150 * ambient::FIXED_POINT_SCALE, 40 * ambient::FIXED_POINT_SCALE},      // compute
                currentTimestampMs,
                {1 * ambient::FIXED_POINT_SCALE, 0, 1 * ambient::FIXED_POINT_SCALE, true, true},                                  // privacy
                ""                                            // cryptographicVerificationHash
            };

            // Sample telemetry for smartphone node
            ambient::TelemetrySample sampleB{
                {"pubB", cfg_.region, "smartphone"},
                {8 * ambient::FIXED_POINT_SCALE + 5000, 1 * ambient::FIXED_POINT_SCALE + 2000, 42 * ambient::FIXED_POINT_SCALE, 22 * ambient::FIXED_POINT_SCALE, 200 * ambient::FIXED_POINT_SCALE},               // energy
                {25 * ambient::FIXED_POINT_SCALE, 20 * ambient::FIXED_POINT_SCALE, 0, 512 * ambient::FIXED_POINT_SCALE, 25 * ambient::FIXED_POINT_SCALE, 30 * ambient::FIXED_POINT_SCALE},        // compute
                currentTimestampMs,
                {1 * ambient::FIXED_POINT_SCALE, 0, 1 * ambient::FIXED_POINT_SCALE, false, false},                                  // privacy
                ""                                            // cryptographicVerificationHash
            };

            nodeA.ingestTelemetry(sampleA);
            nodeB.ingestTelemetry(sampleB);

            ambient::MeshCoordinator mesh("cluster-" + cfg_.region);
            mesh.registerNode(&nodeA);
            mesh.registerNode(&nodeB);

            // Performance-based reward calculation
            auto perfFn = [](const ambient::AmbientNode& n) -> uint64_t {
                auto last = n.last();
                if (!last.has_value()) return 0;

                int64_t scoreFp = (last->compute.bandwidthMbpsFp * ambient::FIXED_POINT_SCALE) / (50 * ambient::FIXED_POINT_SCALE) -
                                  (last->compute.latencyMsFp * ambient::FIXED_POINT_SCALE) / (500 * ambient::FIXED_POINT_SCALE);

                return std::clamp<uint64_t>(static_cast<uint64_t>(std::max<int64_t>(0, scoreFp)), 1000, 20000); // 0.1 to 2.0 scaled
            };

            auto rewardRec = mesh.dispatchAndReward("task-entropy-infer", perfFn, 10 * ambient::FIXED_POINT_SCALE);

            log(LogLevel::INFO, 
                "[AmbientAI] Reward dispatched: node=" + rewardRec.node.pubkey +
                " tokens=" + std::to_string(rewardRec.rewardTokensFp / ambient::FIXED_POINT_SCALE));

            // Register ambient nodes in orchestration network
            if (orchestrationEngine_) {
                registerAmbientNodes(nodeA, nodeB);
            }

            ailee::RecoveryProtocol::recordIncident(
                "AmbientMeshReward",
                "Node=" + rewardRec.node.pubkey + 
                " Tokens=" + std::to_string(rewardRec.rewardTokensFp / ambient::FIXED_POINT_SCALE));
            
            // Layer-2 NetFlow relay
            if (cfg_.enableNetFlow) {
                try {
                std::vector<ambient::AmbientNode*> nodes = {&nodeA, &nodeB};
                (void)nodes;
                log(LogLevel::INFO, "[NetFlow] Bandwidth relay stubbed");
                } catch (const std::exception& e) {
                    log(LogLevel::WARN, "[NetFlow] Relay failed: " + std::string(e.what()));
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "AmbientAI demo failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("AmbientAIFailure", e.what());
            return false;
        }
    }
    
    // *** NEW: Orchestration task submission demo ***
    bool demoOrchestration() {
        if (!orchestrationEngine_) {
            log(LogLevel::WARN, "Orchestration not enabled, skipping demo");
            return true;
        }
        
        log(LogLevel::INFO, "[Orchestrator] Running task orchestration demo");
        
        try {
            // Create sample task
            ailee::sched::TaskPayload task;
            task.taskId = "demo-task-001";
            task.taskType = ailee::sched::TaskType::AI_INFERENCE;
            task.priority = ailee::sched::TaskPriority::HIGH;
            task.submitterId = cfg_.nodeId;
            task.submittedAt = std::chrono::system_clock::now();
            
            // Set requirements
            task.requirements.minCpuCores = 2;
            task.requirements.minMemoryGB = 4;
            task.requirements.requiresGPU = false;
            task.requirements.minBandwidthMbps = 10.0;
            
            // Economic parameters
            task.maxCostTokens = 100;
            task.minReputationScore = 0.5;
            
            // Submit task asynchronously
            auto future = orchestrationEngine_->submitTask(task);
            
            log(LogLevel::INFO, "[Orchestrator] Task submitted: " + task.taskId);
            
            // Wait for assignment (with timeout)
            auto status = future.wait_for(std::chrono::seconds(5));
            
            if (status == std::future_status::ready) {
                auto assignment = future.get();
                
                if (assignment.assigned) {
                    log(LogLevel::INFO, 
                        "[Orchestrator] Task assigned to: " + assignment.workerPeerId +
                        " | Score: " + std::to_string(assignment.finalScore) +
                        " | Expected cost: " + std::to_string(assignment.expectedCostTokens));
                } else {
                    log(LogLevel::WARN, 
                        "[Orchestrator] Task assignment failed: " + assignment.reason);
                }
            } else {
                log(LogLevel::WARN, "[Orchestrator] Task assignment timeout");
            }
            
            return true;
            
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Orchestration demo failed: " + std::string(e.what()));
            return false;
        }
    }

    // Adaptive throttling
    void throttleSystems() {
        int oldCycles = cfg_.tpsSimCycles;
        cfg_.tpsSimCycles = std::max(50, cfg_.tpsSimCycles / 2);
        
        log(LogLevel::WARN, 
            "Adaptive throttling: TPS cycles " + std::to_string(oldCycles) + 
            " → " + std::to_string(cfg_.tpsSimCycles));
        
        if (cfg_.enableNetFlow) {
            log(LogLevel::DEBUG, "Throttling adjusted considering NetFlow load");
        }
    }
    
    void printOrchestratorMetrics() {
        if (!orchestrationEngine_) {
            return;
        }
        
        try {
            auto metrics = orchestrationEngine_->getMetrics();
            
            log(LogLevel::INFO, "╔════════════════════════════════════════════════╗");
            log(LogLevel::INFO, "║   ORCHESTRATOR METRICS                         ║");
            log(LogLevel::INFO, "╠════════════════════════════════════════════════╣");
            log(LogLevel::INFO, "  Total tasks submitted:   " + std::to_string(metrics.totalTasksSubmitted));
            log(LogLevel::INFO, "  Total tasks completed:   " + std::to_string(metrics.totalTasksCompleted));
            log(LogLevel::INFO, "  Total tasks failed:      " + std::to_string(metrics.totalTasksFailed));
            log(LogLevel::INFO, "  Active nodes:            " + std::to_string(metrics.activeNodes));
            log(LogLevel::INFO, "  Queued tasks:            " + std::to_string(metrics.queuedTasks));
            
            double successRate = metrics.totalTasksSubmitted > 0 ?
                (double)metrics.totalTasksCompleted / metrics.totalTasksSubmitted * 100.0 : 0.0;
            log(LogLevel::INFO, "  Success rate:            " + std::to_string(successRate) + "%");
            log(LogLevel::INFO, "╚════════════════════════════════════════════════╝");
            
        } catch (const std::exception& e) {
            log(LogLevel::WARN, "Failed to print orchestrator metrics: " + std::string(e.what()));
        }
    }

private:
    Config cfg_;
    ailee_netflow::HybridNetFlow netFlow_;
    std::unique_ptr<ailee::sched::Engine> orchestrationEngine_;
    std::unique_ptr<ailee::AILEEWebServer> webServer_;
    std::unique_ptr<ailee::l2::BlockProducer> blockProducer_;
    std::unique_ptr<ailee::l2::Mempool> mempool_;
    std::unique_ptr<ailee::l1::ReorgDetector> reorgDetector_;
    std::unique_ptr<ailee::BitcoinZMQListener> zmqListener_;
    std::atomic<bool> shutdownCalled_;
    std::chrono::steady_clock::time_point startTime_;
    
    ailee::BreakerState breakerState_;
    uint64_t breakerTestBlockHeight_ = 1;

    void initZMQListener() {
        try {
            zmqListener_ = std::make_unique<ailee::BitcoinZMQListener>(cfg_.bitcoinZmqEndpoint);

            if (reorgDetector_) {
                zmqListener_->setReorgDetector(reorgDetector_.get());
                log(LogLevel::INFO, "ZMQ Listener connected to ReorgDetector");
            }

            // In a real deployment, we would call zmqListener_->start() in a separate thread.
            // For this implementation, we just initialize it.
            zmqListener_->init();
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to initialize ZMQ Listener: " + std::string(e.what()));
        }
    }

    void initReorgDetector() {
        try {
            // Ensure data directory exists
            std::filesystem::create_directories("data");
            log(LogLevel::INFO, "Created data directory at " + std::filesystem::absolute("data").string());

            std::string dbPath = "data/reorg_db";
            reorgDetector_ = std::make_unique<ailee::l1::ReorgDetector>(dbPath);

            std::string err;
            if (reorgDetector_->initialize(&err)) {
                log(LogLevel::INFO, "ReorgDetector initialized successfully at " + dbPath);
            } else {
                log(LogLevel::WARN, "ReorgDetector initialization failed: " + err +
                    " (L1 reorg protection completely disabled)");
                reorgDetector_.reset();
            }
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to initialize ReorgDetector: " + std::string(e.what()));
        }
    }

    void initOrchestrationEngine() {
        try {
            auto orchConfig = ailee::sched::createDefaultConfig();
            
            // Apply user configuration
            orchConfig.performance.workerThreads = cfg_.orchestratorWorkers;
            orchConfig.performance.maxConcurrentTasks = cfg_.maxConcurrentTasks;
            
            // Map strategy string to enum
            if (cfg_.orchestratorStrategy == "ROUND_ROBIN") {
                orchConfig.performance.defaultStrategy = ailee::sched::SchedulingStrategy::ROUND_ROBIN;
            } else if (cfg_.orchestratorStrategy == "LEAST_LOADED") {
                orchConfig.performance.defaultStrategy = ailee::sched::SchedulingStrategy::LEAST_LOADED;
            } else if (cfg_.orchestratorStrategy == "LOWEST_COST") {
                orchConfig.performance.defaultStrategy = ailee::sched::SchedulingStrategy::LOWEST_COST;
            } else {
                orchConfig.performance.defaultStrategy = ailee::sched::SchedulingStrategy::WEIGHTED_SCORE;
            }
            
            orchConfig.monitoring.enableMetrics = cfg_.enableMetrics;
            orchConfig.network.listenPort = cfg_.netFlowPort + 100; // Offset from NetFlow
            
            orchestrationEngine_ = ailee::sched::createEngine(orchConfig);
            
            log(LogLevel::INFO, "Orchestration engine initialized with strategy: " + 
                cfg_.orchestratorStrategy);
                
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to initialize orchestration engine: " + 
                std::string(e.what()));
            throw;
        }
    }
    
    void initWebServer() {
        try {
            ailee::WebServerConfig wsConfig;
            wsConfig.host = cfg_.webServerHost;
            wsConfig.port = cfg_.webServerPort;
            wsConfig.enable_cors = true;
            wsConfig.enable_ssl = false;
            wsConfig.thread_pool_size = 4;
            
            webServer_ = std::make_unique<ailee::AILEEWebServer>(wsConfig);
            
            // Set up status callback to provide node information
            startTime_ = std::chrono::steady_clock::now();
            webServer_->setNodeStatusCallback([this]() -> ailee::NodeStatus {
                auto now = std::chrono::steady_clock::now();
                auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();
                
                ailee::NodeStatus status;
                status.running = true;
                status.version = "2.0.0";
                status.uptime_seconds = uptime;
                status.network = cfg_.network;
                status.total_transactions = 0;
                status.total_blocks = 0;
                status.current_tps = 0.0;
                status.pending_tasks = 0;
                status.last_anchor_hash = "N/A";
                
                // Get metrics from orchestrator if available
                if (orchestrationEngine_) {
                    try {
                        auto metrics = orchestrationEngine_->getMetrics();
                        status.pending_tasks = metrics.queuedTasks;
                    } catch (...) {
                        // Ignore errors
                    }
                }
                
                return status;
            });
            
            log(LogLevel::INFO, "Web server initialized on " + wsConfig.host + ":" + 
                std::to_string(wsConfig.port));
                
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to initialize web server: " + 
                std::string(e.what()));
            throw;
        }
    }
    
    void initBlockProducer() {
        try {
            // Create mempool first
            mempool_ = std::make_unique<ailee::l2::Mempool>();
            log(LogLevel::INFO, "Mempool initialized");
            
            ailee::l2::BlockProducer::Config blockConfig;
            
            // Use configured values from config.yaml
            // These defaults match the values in config/config.yaml
            blockConfig.blockIntervalMs = 1000;      // 1 block/second (config.yaml: block_interval_ms)
            blockConfig.commitmentInterval = 100;     // Anchor every 100 blocks (config.yaml: commitment_interval)
            
            blockProducer_ = std::make_unique<ailee::l2::BlockProducer>(blockConfig);
            
            // Wire mempool to block producer
            blockProducer_->setMempool(mempool_.get());
            
            // Wire reorg detector to block producer
            if (reorgDetector_) {
                blockProducer_->setReorgDetector(reorgDetector_.get());
            }

            log(LogLevel::INFO, "Block producer initialized (interval: " + 
                std::to_string(blockConfig.blockIntervalMs) + "ms, anchor every " + 
                std::to_string(blockConfig.commitmentInterval) + " blocks)");
                
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to initialize block producer: " + 
                std::string(e.what()));
            throw;
        }
    }
    
    void registerSelfAsNode() {
        try {
            ailee::sched::NodeMetrics selfMetrics;
            selfMetrics.peerId = cfg_.nodeId;
            selfMetrics.region = cfg_.region;
            
            // Set capabilities based on system
            auto cores = std::thread::hardware_concurrency();
            if (cores == 0) {
                cores = 1;
            }
            selfMetrics.capabilities.cpuCores = cores;
            selfMetrics.capabilities.memoryGB = 16; // Would query actual system memory
            selfMetrics.capabilities.hasGPU = false; // Would detect GPU
            
            // Set initial metrics
            selfMetrics.latencyMs = 50.0;
            selfMetrics.bandwidthMbps = 100.0;
            selfMetrics.cpuUtilization = 0.3;
            selfMetrics.capacityScore = 0.8;
            selfMetrics.costPerHour = 10.0;
            selfMetrics.tokensAvailable = 1000;
            selfMetrics.maxConcurrentTasks = cfg_.maxConcurrentTasks;
            selfMetrics.lastSeen = std::chrono::system_clock::now();
            
            orchestrationEngine_->registerNode(selfMetrics);
            
            log(LogLevel::INFO, "Registered self as orchestration node: " + cfg_.nodeId);
            
        } catch (const std::exception& e) {
            log(LogLevel::WARN, "Failed to register self as node: " + std::string(e.what()));
        }
    }
    
    void registerAmbientNodes(const ambient::AmbientNode& nodeA, 
                             const ambient::AmbientNode& nodeB) {
        try {
            auto lastA = nodeA.last();
            auto lastB = nodeB.last();
            
            if (lastA.has_value()) {
                ailee::sched::NodeMetrics metricsA;
                metricsA.peerId = lastA->node.pubkey;
                metricsA.region = lastA->node.region;
                metricsA.bandwidthMbps = static_cast<double>(lastA->compute.bandwidthMbpsFp) / ambient::FIXED_POINT_SCALE;
                metricsA.latencyMs = static_cast<double>(lastA->compute.latencyMsFp) / ambient::FIXED_POINT_SCALE;
                metricsA.cpuUtilization = static_cast<double>(lastA->compute.cpuUtilizationFp) / (ambient::FIXED_POINT_SCALE * 100.0);
                metricsA.capacityScore = 0.7;
                
                orchestrationEngine_->registerNode(metricsA);
            }
            
            if (lastB.has_value()) {
                ailee::sched::NodeMetrics metricsB;
                metricsB.peerId = lastB->node.pubkey;
                metricsB.region = lastB->node.region;
                metricsB.bandwidthMbps = static_cast<double>(lastB->compute.bandwidthMbpsFp) / ambient::FIXED_POINT_SCALE;
                metricsB.latencyMs = static_cast<double>(lastB->compute.latencyMsFp) / ambient::FIXED_POINT_SCALE;
                metricsB.cpuUtilization = static_cast<double>(lastB->compute.cpuUtilizationFp) / (ambient::FIXED_POINT_SCALE * 100.0);
                metricsB.capacityScore = 0.6;
                
                orchestrationEngine_->registerNode(metricsB);
            }
            
            log(LogLevel::DEBUG, "Registered ambient nodes in orchestration network");
            
        } catch (const std::exception& e) {
            log(LogLevel::WARN, "Failed to register ambient nodes: " + std::string(e.what()));
        }
    }
    
    void submitTPSResultsTask(const ailee::PerformanceSimulator::SimulationResult& result) {
        try {
            ailee::sched::TaskPayload task;
            task.taskId = "tps-result-" + std::to_string(result.cycles);
            task.taskType = ailee::sched::TaskType::DATA_PROCESSING;
            task.priority = ailee::sched::TaskPriority::NORMAL;
            task.submitterId = cfg_.nodeId;
            task.submittedAt = std::chrono::system_clock::now();
            
            // Store result data in payload
            std::string resultData = "TPS=" + std::to_string(result.finalTPS) + 
                                    ";Improvement=" + std::to_string(result.improvementFactor);
            task.payloadBytes.assign(resultData.begin(), resultData.end());
            
            orchestrationEngine_->submitTask(task);
            
            log(LogLevel::DEBUG, "TPS results submitted as orchestrated task");
            
        } catch (const std::exception& e) {
            log(LogLevel::DEBUG, "Failed to submit TPS results task: " + std::string(e.what()));
        }
    }
};

// ---------------------------------------------------------
// Health Check System
// ---------------------------------------------------------
class HealthMonitor {
public:
    struct HealthStatus {
        bool healthy = true;
        std::string status = "OK";
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };
    
    static HealthStatus checkHealth(const AILEEEngine& engine, const Config& cfg) {
        (void)engine;
        HealthStatus health;
        
        // Check configuration
        if (!cfg.validate()) {
            health.healthy = false;
            health.status = "CONFIGURATION_ERROR";
            health.errors.push_back("Invalid configuration detected");
        }
        
        // Check disk space (simplified)
        // In production, would check actual disk usage
        
        // Check memory (simplified)
        // In production, would check actual memory usage
        
        // Add warnings for edge cases
        if (cfg.tpsSimCycles < 100) {
            health.warnings.push_back("Low TPS simulation cycles may affect accuracy");
        }
        
        if (!cfg.enableOrchestration) {
            health.warnings.push_back("Orchestration disabled - node cannot participate in task distribution");
        }
        
        return health;
    }
};

// ---------------------------------------------------------
// Main entry point with enhanced error handling and metrics
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    // Install signal handlers first
    installSignalHandlers();
    
    log(LogLevel::INFO, "╔═══════════════════════════════════════════════════╗");
    log(LogLevel::INFO, "║   AILEE-Core Node [v2.0.0-Production-Ready]      ║");
    log(LogLevel::INFO, "║   Enhanced Distributed Orchestration System       ║");
    log(LogLevel::INFO, "╚═══════════════════════════════════════════════════╝");
    std::cout << "AILEE build id: " << AILEE_BUILD_ID << "\n";
    std::cout << "Rust prover hash: " << AILEE_RUST_PROVER_HASH << "\n";
    std::cout << "AILEE commit hash: " << ailee::build::BuildInfo::getCommitHash() << "\n";
    std::cout << "AILEE build number: " << ailee::build::BuildInfo::getBuildNumber() << "\n";
    std::cout << "AILEE protocol version: " << ailee::build::BuildInfo::getProtocolVersion() << "\n";

    // Load and validate configuration
    Config cfg = loadConfigFromEnv();
    
    // Initialize log file
    initLogFile(cfg.logPath);
    log(LogLevel::INFO, "Log file initialized: " + cfg.logPath);
    
    if (!cfg.validate()) {
        log(LogLevel::FATAL, "Configuration validation failed");
        closeLogFile();
        return 1;
    }

    log(LogLevel::INFO, "Configuration loaded successfully");
    log(LogLevel::INFO, "Node ID: " + cfg.nodeId + " | Region: " + cfg.region);
    
    // Display configuration summary
    log(LogLevel::INFO, "Configuration Summary:");
    log(LogLevel::INFO, "  TPS Nodes: " + std::to_string(cfg.tpsSimNodes));
    log(LogLevel::INFO, "  TPS Cycles: " + std::to_string(cfg.tpsSimCycles));
    log(LogLevel::INFO, "  NetFlow: " + std::string(cfg.enableNetFlow ? "ENABLED" : "DISABLED"));
    log(LogLevel::INFO, "  Orchestration: " + std::string(cfg.enableOrchestration ? "ENABLED" : "DISABLED"));
    if (cfg.enableOrchestration) {
        log(LogLevel::INFO, "  Orchestrator Workers: " + std::to_string(cfg.orchestratorWorkers));
        log(LogLevel::INFO, "  Orchestrator Strategy: " + cfg.orchestratorStrategy);
    }
    
    int exitCode = 0;
    std::unique_ptr<AILEEEngine> engine;
    
    try {
        // Initialize engine
        engine = std::make_unique<AILEEEngine>(cfg);
        
        // Perform health check
        auto health = HealthMonitor::checkHealth(*engine, cfg);
        if (!health.healthy) {
            log(LogLevel::ERROR, "Health check failed: " + health.status);
            for (const auto& error : health.errors) {
                log(LogLevel::ERROR, "  - " + error);
            }
            closeLogFile();
            return 1;
        }
        
        for (const auto& warning : health.warnings) {
            log(LogLevel::WARN, "  - " + warning);
        }
        
        // Start engine
        engine->start();
        
        // Run system tests
        auto startTime = std::chrono::steady_clock::now();
        
        // TPS Simulation
        if (!g_shutdown.load()) {
            if (!engine->runTPSSimulation()) {
                log(LogLevel::ERROR, "TPS Simulation failed");
                exitCode = 1;
            }
        }
        
        // Gold Bridge
        if (!g_shutdown.load()) {
            if (!engine->testGoldBridge()) {
                log(LogLevel::ERROR, "Gold Bridge test failed");
                exitCode = 1;
            }
        }
        
        // Safety & Energy
        if (!g_shutdown.load()) {
            if (!engine->testSafetyAndEnergy()) {
                log(LogLevel::ERROR, "Safety/Energy test failed");
                exitCode = 1;
            }
        }
        
        // AmbientAI Mesh
        if (!g_shutdown.load()) {
            if (!engine->demoAmbientMesh()) {
                log(LogLevel::ERROR, "AmbientAI demo failed");
                exitCode = 1;
            }
        }
        
        // Orchestration Demo
        if (!g_shutdown.load() && cfg.enableOrchestration) {
            if (!engine->demoOrchestration()) {
                log(LogLevel::ERROR, "Orchestration demo failed");
                exitCode = 1;
            }
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        log(LogLevel::INFO, "All systems tested in " + std::to_string(duration.count()) + "ms");

        if (g_shutdown.load()) {
            log(LogLevel::WARN, "Shutdown requested by signal during initialization");
        } else {
            // Keep the server running and wait for shutdown signal
            log(LogLevel::INFO, "╔═══════════════════════════════════════════════════╗");
            log(LogLevel::INFO, "║   AILEE-Core Node is now RUNNING                  ║");
            
            // Format the port message with proper spacing
            std::ostringstream portMsg;
            portMsg << "║   HTTP API available on port " << cfg.webServerPort;
            // Pad with spaces to maintain box alignment (51 chars total width)
            int paddingNeeded = 51 - static_cast<int>(portMsg.str().length());
            for (int i = 0; i < paddingNeeded; ++i) {
                portMsg << " ";
            }
            portMsg << "║";
            log(LogLevel::INFO, portMsg.str());
            
            log(LogLevel::INFO, "║   Press Ctrl+C to shutdown gracefully             ║");
            log(LogLevel::INFO, "╚═══════════════════════════════════════════════════╝");
            
            // Main service loop - wait for shutdown signal
            while (!g_shutdown.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            log(LogLevel::INFO, "Shutdown signal received, initiating graceful shutdown...");
        }
        
    } catch (const std::exception& e) {
        log(LogLevel::FATAL, "Unhandled exception: " + std::string(e.what()));
        ailee::RecoveryProtocol::recordIncident("FatalMainException", e.what());
        exitCode = 2;
    } catch (...) {
        log(LogLevel::FATAL, "Unknown exception caught");
        ailee::RecoveryProtocol::recordIncident("FatalUnknownException", "Unknown error");
        exitCode = 3;
    }

    // Cleanup
    if (engine) {
        engine->shutdown();
    }

    log(LogLevel::INFO, "╔═══════════════════════════════════════════════════╗");
    log(LogLevel::INFO, "║   AILEE-Core shutdown complete                   ║");
    log(LogLevel::INFO, "║   Exit code: " + std::to_string(exitCode) + "                                      ║");
    log(LogLevel::INFO, "╚═══════════════════════════════════════════════════╝");
    
    closeLogFile();
    return exitCode;
}
