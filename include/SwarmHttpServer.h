#pragma once

class ProverSwarm;

class SwarmHttpServer {
public:
    // Optional: hook into existing AILEEWebServer to expose /prover/register
    static void attach(ProverSwarm& swarm);
};
