#include "SwarmRuntime.h"
#include "SwarmConfigLoader.h"
#include "ProverRegistry.h"
#include "SwarmConfig.h"

#include "ProverSwarm.h"  // your existing swarm implementation

#include <iostream>

void SwarmRuntime::initialize(ProverSwarm& swarm) {
    SwarmConfig cfg = SwarmConfigLoader::load("config/ailee.toml");

    ProverRegistry::instance().loadFromConfig(cfg);

    if (cfg.provers.empty()) {
        std::cout << "[Swarm] No provers found in config/ailee.toml\n";
        return;
    }

    std::cout << "[Swarm] Loading provers from config...\n";
    for (const auto& p : cfg.provers) {
        // You may need to adjust this depending on ProverSwarm API.
        // Example: swarm.addProver(p.pubkey, p.capacity, p.latency_ms, p.reputation);
        swarm.addProver(p.pubkey, p.capacity, p.latency_ms, p.reputation);

        std::cout << "[Swarm] Loaded prover " << p.pubkey
                  << " (capacity=" << p.capacity
                  << ", latency_ms=" << p.latency_ms
                  << ", reputation=" << p.reputation
                  << ")\n";
    }
}
