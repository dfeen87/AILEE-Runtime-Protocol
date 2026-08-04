// Licensed under the PolyForm Noncommercial License 1.0.0
// Ambient AI + VCP Integration Demo
// Demonstrates end-to-end workflow: Task submission → Node selection → WASM execution → ZK verification → Reward distribution

#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>
#include "../include/AmbientAI.h"
#include "../src/runtime/WasmEngine.h"

using namespace ambient;
using namespace ailee::exec;

// ==================== DEMO CONFIGURATION ====================

struct DemoConfig {
    int numNodes = 3;
    int numTasks = 5;
    bool verbose = true;
};

// ==================== SIMULATED WASM ENGINE ====================
// This is a stub implementation for demonstration purposes
// In production, this would integrate with WasmEdge, Wasmer, etc.

class SimulatedWasmEngine {
public:
    WasmResult execute(const WasmCall& call, const SandboxLimits& limits) {
        WasmResult result;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate computation (simple hash-based work)
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 500)));
        
        auto end = std::chrono::high_resolution_clock::now();
        
        // Populate result
        result.success = true;
        result.outputBytes = {0x01, 0x02, 0x03, 0x04}; // Dummy output
        result.outputHash = "sha256:abcd1234..."; // Would be real hash
        result.moduleHash = "sha256:module123...";
        result.executionHash = "sha256:exec456...";
        
        // Metrics
        result.metrics.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        result.metrics.peakMemoryUsed = 1024 * 1024 * (10 + rand() % 40); // 10-50MB
        result.metrics.instructionsExecuted = 1000000 + rand() % 9000000;
        result.metrics.gasConsumed = result.metrics.instructionsExecuted / 10;
        result.metrics.functionCallCount = 100 + rand() % 900;
        
        // Simulated ZK proof
        result.zkProof = "proof:0x" + std::to_string(rand());
        result.zkVerified = (rand() % 100 < 95); // 95% success rate
        
        return result;
    }
};

// ==================== DEMO ORCHESTRATOR ====================
// Simplified orchestrator that works with the actual AmbientNode API

class AmbientVCPOrchestrator {
private:
    std::vector<std::unique_ptr<AmbientNode>> nodes;
    std::vector<NodeId> nodeIds;
    std::vector<TelemetrySample> telemetryData;
    MeshCoordinator meshCoordinator;
    SimulatedWasmEngine wasmEngine;
    
    double calculateNodeScore(const TelemetrySample& telemetry, const Reputation& rep) {
        // Multi-factor health scoring
        double bandwidthScore = std::min(telemetry.compute.bandwidthMbps / 1000.0, 1.0);
        double latencyScore = std::max(0.0, 1.0 - telemetry.compute.latencyMs / 100.0);
        double efficiencyScore = std::min(telemetry.energy.computeEfficiency_GFLOPS_W / 10.0, 1.0);
        double reputationScore = rep.score;
        
        return (bandwidthScore * 0.40) +
               (latencyScore * 0.30) +
               (efficiencyScore * 0.20) +
               (reputationScore * 0.10);
    }
    
public:
    AmbientVCPOrchestrator() : meshCoordinator("demo-cluster") {}
    
    void initializeNodes(int count) {
        std::cout << "\n=== Initializing Ambient Node Mesh ===" << std::endl;
        
        for (int i = 0; i < count; i++) {
            // Create node ID
            NodeId id;
            id.pubkey = "node_" + std::to_string(i);
            id.region = (i % 2 == 0) ? "us-west" : "eu-central";
            id.deviceClass = (i == 0) ? "gateway" : ((i == 1) ? "smartphone" : "pc");
            
            // Create safety policy
            SafetyPolicy policy;
            policy.maxTemperatureC = 85.0;
            policy.maxLatencyMs = 100.0;
            policy.maxBlockMB = 8.0;
            policy.maxErrorCount = 25;
            
            // Create node with proper constructor
            auto node = std::make_unique<AmbientNode>(id, policy);
            
            // Create telemetry sample
            TelemetrySample telemetry;
            telemetry.node = id;
            telemetry.timestamp = std::chrono::system_clock::now();
            
            // Randomize telemetry data
            telemetry.energy.inputPowerW = 30.0 + rand() % 70;
            telemetry.energy.temperatureC = 40.0 + rand() % 30;
            telemetry.energy.computeEfficiency_GFLOPS_W = 5.0 + (rand() % 50) / 10.0;
            telemetry.energy.carbonIntensity_gCO2_kWh = 200 + rand() % 300;
            
            telemetry.compute.cpuUtilization = 0.2 + (rand() % 50) / 100.0;
            telemetry.compute.availableMemMB = 1024 + rand() % 7168;
            telemetry.compute.bandwidthMbps = 100.0 + rand() % 900;
            telemetry.compute.latencyMs = 5.0 + rand() % 45;
            
            // Ingest telemetry into node
            node->ingestTelemetry(telemetry);
            
            // Initialize reputation
            for (int j = 0; j < (rand() % 50); j++) {
                node->updateReputation(true, 0.01);
            }
            
            Reputation rep = node->reputation();
            meshCoordinator.registerNode(node.get());
            
            std::cout << "  ✓ Node " << i << " (" << id.deviceClass << "): "
                      << "Health=" << std::fixed << std::setprecision(2) << calculateNodeScore(telemetry, rep)
                      << ", Power=" << telemetry.energy.inputPowerW << "W"
                      << ", Latency=" << telemetry.compute.latencyMs << "ms" << std::endl;
            
            nodeIds.+= id);
            telemetryData.+= telemetry);
            nodes.+= std::move(node));
        }
    }
    
    void executeTask(const std::string& taskId, const std::string& taskDescription) {
        std::cout << "\n--- Task: " << taskId << " ---" << std::endl;
        std::cout << "Description: " << taskDescription << std::endl;
        
        // 1. Select best node based on health scoring
        double bestScore = -1.0;
        size_t bestIdx = 0;
        
        for (size_t i = 0; i < nodes.size(); i++) {
            Reputation rep = nodes[i]->reputation();
            double score = calculateNodeScore(telemetryData[i], rep);
            if (score > bestScore) {
                bestScore = score;
                bestIdx = i;
            }
        }
        
        auto& bestNode = nodes[bestIdx];
        auto& bestTelemetry = telemetryData[bestIdx];
        
        std::cout << "  → Selected: " << nodeIds[bestIdx].pubkey
                  << " (Health: " << std::fixed << std::setprecision(3) << bestScore << ")" << std::endl;
        
        // 2. Prepare WASM call
        WasmCall call;
        call.functionName = "run_inference";
        call.inputBytes = {0x00, 0x01, 0x02, 0x03}; // Dummy input
        call.inputHash = "sha256:input123...";
        call.nodeId = nodeIds[bestIdx].pubkey;
        
        SandboxLimits limits;
        // Use defaults from WasmEngine.h
        
        std::cout << "  → Executing in WASM sandbox..." << std::endl;
        
        // 3. Execute with telemetry
        auto execStart = std::chrono::high_resolution_clock::now();
        WasmResult result = wasmEngine.execute(call, limits);
        auto execEnd = std::chrono::high_resolution_clock::now();
        
        // 4. Verify ZK proof
        std::cout << "  → ZK Proof: " << (result.zkVerified ? "✓ VERIFIED" : "✗ FAILED") << std::endl;
        
        // 5. Update telemetry (simulate work impact)
        bestTelemetry.compute.cpuUtilization = 0.8;
        bestTelemetry.energy.inputPowerW += 10.0;
        bestNode->ingestTelemetry(bestTelemetry);
        
        // 6. Calculate reward
        double baseReward = 0.1;
        double efficiencyBonus = 0.0;
        
        if (result.metrics.executionTime.count() < 500000) { // < 500ms
            efficiencyBonus = 0.02;
        }
        
        double totalReward = baseReward + efficiencyBonus;
        
        // 7. Update reputation
        if (result.zkVerified) {
            bestNode->updateReputation(true, 0.01);
            bestNode->accrueReward(taskId, totalReward);
            
            std::cout << "  → Reward: " << std::fixed << std::setprecision(4) << totalReward << " AILEE tokens" << std::endl;
        } else {
            bestNode->updateReputation(false, 0.05);
            std::cout << "  → Penalty: Reputation decreased" << std::endl;
        }
        
        // 8. Display metrics
        std::cout << "  → Metrics:" << std::endl;
        std::cout << "      Execution Time: " << result.metrics.executionTime.count() / 1000.0 << " ms" << std::endl;
        std::cout << "      Memory Used: " << result.metrics.peakMemoryUsed / (1024 * 1024) << " MB" << std::endl;
        std::cout << "      Instructions: " << result.metrics.instructionsExecuted << std::endl;
        std::cout << "      Gas Consumed: " << result.metrics.gasConsumed << std::endl;
        std::cout << "  → Energy Impact:" << std::endl;
        std::cout << "      Power: " << bestTelemetry.energy.inputPowerW << " W" << std::endl;
        std::cout << "      Temperature: " << bestTelemetry.energy.temperatureC << " °C" << std::endl;
        std::cout << "      Efficiency: " << bestTelemetry.energy.computeEfficiency_GFLOPS_W << " GFLOPS/W" << std::endl;
    }
    
    void printSummary() {
        std::cout << "\n=== Mesh Summary ===" << std::endl;
        
        double totalTasks = 0;
        double totalEnergy = 0.0;
        
        for (size_t i = 0; i < nodes.size(); i++) {
            Reputation rep = nodes[i]->reputation();
            totalTasks += rep.completedTasks;
            totalEnergy += telemetryData[i].energy.inputPowerW;
            
            std::cout << "\n" << nodeIds[i].pubkey << ":" << std::endl;
            std::cout << "  Device: " << nodeIds[i].deviceClass << " (" << nodeIds[i].region << ")" << std::endl;
            std::cout << "  Reputation: " << std::fixed << std::setprecision(3) << rep.score << std::endl;
            std::cout << "  Tasks Completed: " << rep.completedTasks << std::endl;
            std::cout << "  Current Power: " << telemetryData[i].energy.inputPowerW << " W" << std::endl;
            std::cout << "  Health Score: " << calculateNodeScore(telemetryData[i], rep) << std::endl;
        }
        
        std::cout << "\nNetwork Totals:" << std::endl;
        std::cout << "  Total Tasks: " << totalTasks << std::endl;
        std::cout << "  Total Power Draw: " << totalEnergy << " W" << std::endl;
        std::cout << "  Average Power per Node: " << totalEnergy / nodes.size() << " W" << std::endl;
    }
};

// ==================== MAIN DEMO ====================

int main(int argc, char* argv[]) {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════╗
║                                                                  ║
║        Ambient AI + VCP Integration Demo                        ║
║        Decentralized Verifiable Computation on Bitcoin L2       ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
)" << std::endl;

    srand(time(nullptr));
    
    DemoConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--nodes" && i + 1 < argc) {
            config.numNodes = std::stoi(argv[++i]);
        } else if (arg == "--tasks" && i + 1 < argc) {
            config.numTasks = std::stoi(argv[++i]);
        } else if (arg == "--quiet") {
            config.verbose = false;
        }
    }
    
    // Initialize orchestrator
    AmbientVCPOrchestrator orchestrator;
    
    // Set up mesh network
    orchestrator.initializeNodes(config.numNodes);
    
    // Execute sample tasks
    std::cout << "\n=== Executing Tasks ===" << std::endl;
    
    std::vector<std::pair<std::string, std::string>> tasks = {
        {"task_001", "ML Inference: Image Classification (cat.jpg)"},
        {"task_002", "FL Training: MNIST Model Update (Local Data)"},
        {"task_003", "Data Processing: JSON Transform (10MB dataset)"},
        {"task_004", "Cryptographic: ZK Proof Generation (circuit_001)"},
        {"task_005", "Analytics: Time Series Analysis (IoT sensor data)"}
    };
    
    for (int i = 0; i < std::min(config.numTasks, static_cast<int>(tasks.size())); i++) {
        orchestrator.executeTask(tasks[i].first, tasks[i].second);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Print final summary
    orchestrator.printSummary();
    
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════╗
║                                                                  ║
║  Demo Complete!                                                  ║
║                                                                  ║
║  Next Steps:                                                     ║
║  1. Implement real WasmEngine with WasmEdge/Wasmer              ║
║  2. Add ZK proof generation with RISC Zero or Plonky2           ║
║  3. Connect to Bitcoin L2 for token settlement                  ║
║  4. Deploy multi-node mesh on real network                      ║
║                                                                  ║
║  See docs/AMBIENT_VCP_INTEGRATION.md for full roadmap           ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
)" << std::endl;

    return 0;
}
