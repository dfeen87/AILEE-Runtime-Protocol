#include "ailee_api.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ailee;

int main() {
    std::cout << "Starting AILEE Node Sidecar Simulation...\n";

    AileeConfig config;
    config.environment = "simulation";
    config.strict_mode = true;

    AileeContext ctx = ailee_init(config);

    // Simulate continuously polling a node
    for (int i = 0; i < 3; ++i) {
        std::cout << "\n[Polling Node - Iteration " << (i+1) << "]\n";

        // Mock generating snapshot from local node data
        BitcoinSnapshot snap;
        snap.height = 800000 + i;
        snap.price_context = {65000.0 + (i * 100), 1.2 + (i * 0.1), 0.2};
        snap.mempool_stats = {1000000 + static_cast<uint64_t>(i * 50000), 5000 + static_cast<uint32_t>(i * 200), 15.0, 30.0, 5.0};
        snap.current_fee_rate = 18.0 + i;
        snap.hashrate_estimate = 550.0;
        snap.recent_volatility = 0.2 + (i * 0.05);
        snap.dominance_or_regime_tag = "neutral";
        snap.time_since_last_block = 120 + (i * 50);
        snap.block_interval_avg = 590 + (i * 5);

        PostureReport report = ailee_evaluate_posture(snap, ctx);

        std::cout << "Current Risk Score: " << report.risk.value << std::endl;
        std::cout << "Regime Evaluated: " << report.regime << std::endl;
        std::cout << "Summary: " << report.summary << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nSidecar Simulation Complete.\n";

    ailee_shutdown(ctx);
    return 0;
}
