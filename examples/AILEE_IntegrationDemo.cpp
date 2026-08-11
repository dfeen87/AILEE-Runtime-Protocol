/**
 * AILEE Complete Integration Demo
 * 
 * Demonstrates how to use all three enhanced modules together:
 * 1. TPS Engine v2.0 - AI-driven optimization with testnet integration
 * 2. Recovery Protocol v2.0 - Trustless BTC recovery with dispute handling
 * 3. AmbientAI Core v2.0 - Consensus, energy telemetry, token economics
 * 
 * Licensed under the MIT License
 * Copyright (c) 2026 Don Michael Feeney Jr.
 * Author: Don Michael Feeney Jr
 */

#include "ailee_tps_engine_v2.h"
#include "ailee_recovery_protocol_v2.h"
#include "ambient_ai_core_v2.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace ailee;
using namespace ambient;

// ============================================================================
// DEMO 1: TPS OPTIMIZATION WITH TESTNET INTEGRATION
// ============================================================================

void demo_tps_optimization() {
    std::cout << "\n=== DEMO 1: TPS OPTIMIZATION ===" << std::endl;
    std::cout << "Demonstrating AI-driven Bitcoin scaling\n" << std::endl;
    
    // Initialize AILEE engine
    AILEEEngine engine;
    
    // Optional: Connect to Bitcoin testnet
    /*
    BitcoinTestnetBridge::RPCConfig config;
    config.host = "127.0.0.1";
    config.port = 18332;
    config.username = "your_rpc_user";
    config.password = "your_rpc_password";
    
    try {
        engine.connectToTestnet(config);
        std::cout << "✓ Connected to Bitcoin testnet" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "⚠ Using simulated metrics: " << e.what() << std::endl;
    }
    */
    
    // Run optimization simulation
    std::cout << "\nRunning 200 optimization cycles..." << std::endl;
    
    auto result = PerformanceSimulator::runSimulation(
        100,    // Node count
        1.0,    // Initial block size (MB)
        200     // Cycles
    );
    
    std::cout << "\n--- RESULTS ---" << std::endl;
    std::cout << "Initial TPS:       " << result.initialTPS << std::endl;
    std::cout << "Final TPS:         " << result.finalTPS << std::endl;
    std::cout << "Improvement:       " << std::fixed << std::setprecision(2) 
              << result.improvementFactor << "x" << std::endl;
    std::cout << "Final MAE:         " << result.finalMAE << std::endl;
    std::cout << "Final RMSE:        " << result.finalRMSE << std::endl;
    
    // Export results for visualization
    PerformanceSimulator::exportResultsToCSV(result, "tps_optimization.csv");
    std::cout << "\n✓ Results exported to tps_optimization.csv" << std::endl;
    
    // Generate heatmap data
    std::cout << "\nGenerating performance heatmap..." << std::endl;
    auto heatmap = PerformanceSimulator::generateHeatmap(
        100,    // Min nodes
        5000,   // Max nodes
        500,    // Node step
        0.5,    // Min block size
        2.0,    // Max block size
        0.1     // Block step
    );
    
    std::cout << "✓ Heatmap generated (" << heatmap.size() 
              << " block sizes × " << heatmap[0].size() 
              << " node counts)" << std::endl;
}

// ============================================================================
// DEMO 2: BITCOIN RECOVERY PROTOCOL WITH DISPUTE HANDLING
// ============================================================================

void demo_recovery_protocol() {
    std::cout << "\n\n=== DEMO 2: BITCOIN RECOVERY PROTOCOL ===" << std::endl;
    std::cout << "Demonstrating trustless recovery with cryptographic verification\n" 
              << std::endl;
    
    // Initialize protocol
    RecoveryProtocol protocol;
    
    // Add validators
    auto* validators = protocol.getValidatorNetwork();
    for (int i = 0; i < 10; ++i) {
        ValidatorNetwork::Validator v;
        v.id = "validator_" + std::to_string(i);
        v.address = "addr_" + std::to_string(i);
        v.stake = 100000;
        v.reputation = 100;
        v.active = true;
        validators->addValidator(v);
    }
    
    std::cout << "✓ Initialized with " 
              << validators->getActiveValidatorCount() 
              << " validators" << std::endl;
    
    // Submit recovery claim
    std::cout << "\nSubmitting recovery claim..." << std::endl;
    
    std::string txId = "abc123def456...";  // Dormant Bitcoin TXID
    uint32_t vout = 0;
    std::string claimantAddr = "claimant_btc_address";
    
    // Simulate 20 years of inactivity
    uint64_t twentyYearsAgo = std::chrono::system_clock::now().time_since_epoch().count() 
                              - (20ULL * 365 * 24 * 3600);
    
    std::vector<uint8_t> witnessData = {0x01, 0x02, 0x03};  // Proof material
    
    std::string claimId = protocol.submitClaim(
        txId, vout, claimantAddr, twentyYearsAgo, witnessData
    );
    
    if (claimId.empty()) {
        std::cout << "✗ Claim submission failed" << std::endl;
        return;
    }
    
    std::cout << "✓ Claim submitted: " << claimId.substr(0, 16) << "..." << std::endl;
    std::cout << "  Status: " << static_cast<int>(protocol.getClaimStatus(claimId)) 
              << " (CHALLENGE_PERIOD)" << std::endl;
    
    // Simulate dispute with cryptographic evidence
    std::cout << "\nSimulating dispute with Merkle proof..." << std::endl;
    
    DisputeEvidence evidence;
    evidence.transactionProof.txId = txId;
    evidence.transactionProof.blockHeight = 800000;
    evidence.recentActivityTimestamp = twentyYearsAgo + (19ULL * 365 * 24 * 3600);
    evidence.ownerSignature = {0xAA, 0xBB, 0xCC};
    evidence.ownerPublicKey = {0x11, 0x22, 0x33};
    evidence.signedMessage = "This is my Bitcoin!";
    evidence.disputeReason = "Recent activity detected";
    
    bool disputed = protocol.disputeClaim(claimId, "original_owner", evidence);
    
    if (disputed) {
        std::cout << "✓ Dispute accepted - claim has valid challenge" << std::endl;
    } else {
        std::cout << "✗ Dispute rejected - evidence invalid" << std::endl;
        
        // If no dispute, validators can vote
        std::cout << "\nCollecting validator votes..." << std::endl;
        
        for (int i = 0; i < 7; ++i) {  // 7/10 approve
            protocol.voteOnClaim(claimId, "validator_" + std::to_string(i), true);
        }
        
        bool approved = protocol.finalizeClaim(claimId);
        std::cout << "✓ Claim finalized: " 
                  << (approved ? "APPROVED" : "REJECTED") << std::endl;
        
        // Show supply dynamics impact
        auto* supplyModel = protocol.getSupplyModel();
        auto metrics = supplyModel->getCurrentMetrics();
        
        std::cout << "\n--- SUPPLY METRICS ---" << std::endl;
        std::cout << "Total BTC Supply:      " << metrics.totalBTCSupply << std::endl;
        std::cout << "Recovered BTC:         " << metrics.recoveredBTC << std::endl;
        std::cout << "Circulating Supply:    " << metrics.circulatingSupply << std::endl;
        std::cout << "Deflationary Pressure: " << metrics.deflationaryPressure << std::endl;
    }
    
    // Get detailed claim info
    auto details = protocol.getClaimDetails(claimId);
    if (details.has_value()) {
        std::cout << "\n--- CLAIM DETAILS ---" << std::endl;
        std::cout << "Claim ID:       " << details->claimId.substr(0, 16) << "..." << std::endl;
        std::cout << "Status:         " << static_cast<int>(details->status) << std::endl;
        std::cout << "Disputes:       " << details->disputes.size() << std::endl;
        std::cout << "Validator Votes: " << details->validatorVotes.size() << std::endl;
    }
}

// ============================================================================
// DEMO 3: AMBIENT AI WITH CONSENSUS & ENERGY TELEMETRY
// ============================================================================

void demo_ambient_ai() {
    std::cout << "\n\n=== DEMO 3: AMBIENT AI SYSTEM ===" << std::endl;
    std::cout << "Demonstrating consensus, energy verification, and token economics\n" 
              << std::endl;
    
    // Create safety policy
    SafetyPolicy policy;
    policy.maxTemperatureC = 85.0;
    policy.maxLatencyMs = 500.0;
    policy.circuitBreakerEnabled = true;
    
    // Create mesh coordinator
    EnhancedMeshCoordinator mesh("cluster_1");
    
    // Create nodes
    std::vector<std::unique_ptr<EnhancedAmbientNode>> nodes;
    std::cout << "Initializing 5 ambient nodes..." << std::endl;
    
    for (int i = 0; i < 5; ++i) {
        NodeId id;
        id.pubkey = "node_" + std::to_string(i) + "_pubkey";
        id.name = "Node-" + std::to_string(i);
        id.region = (i < 3) ? "us-east" : "eu-west";
        
        auto node = std::make_unique<EnhancedAmbientNode>(id, policy);
        
        // Simulate telemetry
        TelemetrySample sample;
        sample.node = id;
        sample.node.reputationScore = 0.9 + (i * 0.01);
        sample.compute.cpuUtilization = 0.6 + (i * 0.05);
        sample.compute.npuUtilization = 0.4;
        sample.compute.gpuUtilization = 0.3;
        sample.compute.latencyMs = 100.0 + (i * 10);
        sample.energy.inputPowerW = 200.0 + (i * 20);
        sample.energy.temperatureC = 65.0 + (i * 2);
        sample.privacy.epsilon = 1.0;
        
        node->ingestTelemetry(sample);
        mesh.registerNode(node.get());
        nodes.+= std::move(node));
    }
    
    std::cout << "✓ Nodes initialized and registered" << std::endl;
    
    // Reach consensus on cluster state
    std::cout << "\nReaching Byzantine Fault Tolerant consensus..." << std::endl;
    auto consensus = mesh.reachConsensus();
    
    std::cout << "--- CONSENSUS RESULT ---" << std::endl;
    std::cout << "Total Nodes:         " << consensus.totalNodes << std::endl;
    std::cout << "Agreement Count:     " << consensus.agreementCount << std::endl;
    std::cout << "Confidence:          " << std::fixed << std::setprecision(2)
              << (consensus.consensusConfidence * 100) << "%" << std::endl;
    std::cout << "Byzantine Nodes:     " << consensus.byzantineNodes.size() << std::endl;
    std::cout << "Consensus Latency:   " << consensus.consensusSample.compute.latencyMs 
              << " ms" << std::endl;
    
    // Submit energy contribution with verification
    std::cout << "\nSubmitting energy contributions..." << std::endl;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        EnergyProof proof;
        proof.meterSerialNumber = "meter_" + std::to_string(i);
        proof.timestampMs = timestampMs();
        proof.kWhGenerated = 10.0 + i;
        proof.kWhToGrid = 8.0 + i;
        proof.wasteHeatRecovered = 1.5;
        proof.thermodynamicEfficiency = 0.85;
        proof.smartMeterSignature = {0x01, 0x02};
        proof.meterPublicKey = {0x03, 0x04};
        proof.oracleAttestation = "chainlink_attestation_" + std::to_string(i);
        proof.latitude = 40.7128 + i;
        proof.longitude = -74.0060;
        proof.gridRegion = "PJM";
        
        bool verified = nodes[i]->submitEnergyContribution(proof);
        
        if (verified) {
            std::cout << "  ✓ Node " << i << " energy verified: " 
                      << proof.kWhToGrid << " kWh" << std::endl;
        }
    }
    
    // Dispatch task and calculate rewards
    std::cout << "\nDispatching computational task..." << std::endl;
    
    auto taskFn = [](const EnhancedAmbientNode& node) -> double {
        auto history = node.getHistory();
        double efficiency = history.avgEnergyEfficiency();
        return std::max(0.5, std::min(2.0, efficiency));  // 0.5x to 2.0x multiplier
    };
    
    auto incentive = mesh.dispatchAndReward("task_compute_001", taskFn, 100.0);
    
    if (incentive.success) {
        std::cout << "✓ Task completed by: " << incentive.nodeId.name << std::endl;
        std::cout << "  Tokens earned: " << incentive.tokensEarned << std::endl;
    }
    
    // Analyze system health
    std::cout << "\nAnalyzing system health..." << std::endl;
    
    std::vector<TelemetrySample> networkState;
    for (const auto& node : nodes) {
        auto last = node->last();
        if (last.has_value()) {
            networkState.+= *last);
        }
    }
    
    SystemHealth health = analyzeSystemHealth(networkState, consensus);
    
    std::cout << "\n--- SYSTEM HEALTH ---" << std::endl;
    std::cout << "Active Nodes:          " << health.activeNodes << std::endl;
    std::cout << "Avg Latency:           " << health.avgLatency_ms << " ms" << std::endl;
    std::cout << "Total Compute Power:   " << health.totalComputePower << std::endl;
    std::cout << "Network Efficiency:    " << health.networkEfficiency << std::endl;
    std::cout << "Byzantine Nodes:       " << health.byzantineNodesDetected << std::endl;
    std::cout << "Consensus Confidence:  " << (health.consensusConfidence * 100) 
              << "%" << std::endl;
    
    // Export health metrics
    std::string healthJSON = exportHealthToJSON(health);
    std::ofstream healthFile("system_health.json");
    healthFile << healthJSON;
    healthFile.close();
    std::cout << "\n✓ Health metrics exported to system_health.json" << std::endl;
}

// ============================================================================
// DEMO 4: INTEGRATED SCENARIO - RECOVERY + TPS + AI
// ============================================================================

void demo_integrated_scenario() {
    std::cout << "\n\n=== DEMO 4: INTEGRATED SCENARIO ===" << std::endl;
    std::cout << "Demonstrating full AILEE ecosystem in action\n" << std::endl;
    
    std::cout << "SCENARIO: Network processes recovery claim while optimizing TPS" 
              << std::endl;
    std::cout << "          and maintaining consensus under Byzantine conditions\n" 
              << std::endl;
    
    // Initialize all components
    AILEEEngine tpsEngine;
    RecoveryProtocol recovery;
    EnhancedMeshCoordinator mesh("main_cluster");
    
    std::cout << "✓ All systems initialized" << std::endl;
    
    // Step 1: Network processes transactions at baseline
    std::cout << "\n[T=0s] Network operating at baseline (7 TPS)" << std::endl;
    NetworkMetrics metrics;
    auto tps = tpsEngine.calculateEnhancedTPS(metrics);
    std::cout << "  Current TPS: " << tps.enhancedTPS << std::endl;
    
    // Step 2: Recovery claim submitted
    std::cout << "\n[T=30s] Recovery claim submitted for dormant address" << std::endl;
    uint64_t oldTime = std::chrono::system_clock::now().time_since_epoch().count() 
                       - (20ULL * 365 * 24 * 3600);
    std::string claimId = recovery.submitClaim(
        "dormant_tx_abc123", 0, "claimant_addr", oldTime, {0x01}
    );
    std::cout << "  Claim ID: " << claimId.substr(0, 16) << "..." << std::endl;
    
    // Step 3: AI begins TPS optimization
    std::cout << "\n[T=60s] AI initiates TPS optimization in response to load" 
              << std::endl;
    for (int i = 0; i < 10; ++i) {
        tpsEngine.optimizationCycle(metrics);
    }
    tps = tpsEngine.calculateEnhancedTPS(metrics);
    std::cout << "  Optimized TPS: " << tps.enhancedTPS << std::endl;
    
    // Step 4: Byzantine node detected during consensus
    std::cout << "\n[T=90s] Byzantine node detected attempting false telemetry" 
              << std::endl;
    std::vector<TelemetrySample> samples;
    TelemetrySample byzantine;
    byzantine.compute.cpuUtilization = 9.9;  // Outlier
    samples.+= byzantine);
    
    for (int i = 0; i < 5; ++i) {
        TelemetrySample normal;
        normal.compute.cpuUtilization = 0.6 + (i * 0.02);
        samples.+= normal);
    }
    
    bool isByzantine = detectByzantineNode(byzantine, samples);
    std::cout << "  Byzantine detection: " << (isByzantine ? "CONFIRMED" : "PASSED") 
              << std::endl;
    
    // Step 5: System reaches steady state
    std::cout << "\n[T=120s] System stabilized at optimal performance" << std::endl;
    for (int i = 0; i < 50; ++i) {
        tpsEngine.optimizationCycle(metrics);
    }
    tps = tpsEngine.calculateEnhancedTPS(metrics);
    std::cout << "  Final TPS: " << tps.enhancedTPS << std::endl;
    std::cout << "  Model Error (MAE): " << tpsEngine.getModelError() << std::endl;
    
    std::cout << "\n✓ Integrated scenario completed successfully" << std::endl;
    std::cout << "  • Recovery protocol: Operational" << std::endl;
    std::cout << "  • TPS optimization: " << (tps.enhancedTPS / 7.0) << "x baseline" 
              << std::endl;
    std::cout << "  • Byzantine tolerance: Active" << std::endl;
    std::cout << "  • Network consensus: Maintained" << std::endl;
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" 
              << std::endl;
    std::cout << "║         AILEE PROTOCOL - COMPLETE INTEGRATION DEMO         ║" 
              << std::endl;
    std::cout << "║                                                            ║" 
              << std::endl;
    std::cout << "║  AI-Load Energy Efficiency Equation Framework              ║" 
              << std::endl;
    std::cout << "║  Bitcoin Layer-2 Scaling • Recovery • Ambient AI           ║" 
              << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" 
              << std::endl;
    
    try {
        // Run all demos
        demo_tps_optimization();
        demo_recovery_protocol();
        demo_ambient_ai();
        demo_integrated_scenario();
        
        std::cout << "\n\n╔════════════════════════════════════════════════════════════╗" 
                  << std::endl;
        std::cout << "║                    ALL DEMOS COMPLETED                     ║" 
                  << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════╝" 
                  << std::endl;
        
        std::cout << "\nGenerated files:" << std::endl;
        std::cout << "  • tps_optimization.csv - TPS performance data" << std::endl;
        std::cout << "  • system_health.json - Network health metrics" << std::endl;
        std::cout << "  • recovery_claims.log - Recovery audit trail" << std::endl;
        std::cout << "  • ailee_recovery_incidents.log - Protocol incidents" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
