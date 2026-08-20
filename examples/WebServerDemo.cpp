// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// AILEE Web Integration Demo - Demonstrates web server functionality

#include "AILEEWebServer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    (void)signal;
    std::cout << "\n[WebDemo] Shutdown signal received..." << std::endl;
    g_running = false;
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=============================================================\n";
    std::cout << "   AILEE Protocol Core - Web Integration Demo                \n";
    std::cout << "=============================================================\n";
    std::cout << "Starting AILEE web server for worldwide integration...\n\n";

    // Configure web server
    ailee::WebServerConfig config;
    config.host = "0.0.0.0";  // Listen on all interfaces
    config.port = 8080;
    config.enable_cors = true;  // Enable CORS for web browser access
    config.thread_pool_size = 4;
    // config.api_key = "your-api-key-here";  // Uncomment for API key auth

    // Create web server instance
    ailee::AILEEWebServer webServer(config);

    // Set up a callback to provide node status
    auto start_time = std::chrono::system_clock::now();
    webServer.setNodeStatusCallback([start_time]() -> ailee::NodeStatus {
        auto now = std::chrono::system_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        
        ailee::NodeStatus status;
        status.running = true;
        status.version = "36.0.0-web-enabled";
        status.uptime_seconds = uptime.count();
        status.total_transactions = 0;  // Simulated
        status.total_blocks = 0;         // Simulated
        status.network = "Bitcoin Mainnet";
        status.current_tps = 0.0;
        status.pending_tasks = 0;
        status.last_anchor_hash = "0000000000000000000000000000000000000000000000000000000000000000";
        
        return status;
    });

    // Start the web server
    if (!webServer.start()) {
        std::cerr << "[WebDemo] ERROR: Failed to start web server!" << std::endl;
        return 1;
    }

    std::cout << "[WebDemo] Web server started successfully!\n";
    std::cout << "[WebDemo] Access points:\n";
    std::cout << "  - Dashboard:     http://localhost:" << config.port << "/\n";
    std::cout << "  - API Status:    http://localhost:" << config.port << "/api/status\n";
    std::cout << "  - API Metrics:   http://localhost:" << config.port << "/api/metrics\n";
    std::cout << "  - Health Check:  http://localhost:" << config.port << "/api/health\n";
    std::cout << "\n[WebDemo] AILEE is now integrated across the world wide web!\n";
    std::cout << "[WebDemo] Press Ctrl+C to stop the server...\n\n";

    // Keep the server running
    while (g_running && webServer.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[WebDemo] Stopping web server...\n";
    webServer.stop();
    std::cout << "[WebDemo] Server stopped. Goodbye!\n";

    return 0;
}
