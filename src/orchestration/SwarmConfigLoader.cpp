#include "SwarmConfigLoader.h"
#include "SwarmConfig.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// Very minimal, line-based parser for a [[provers]] block in ailee.toml.
// You can replace this with your existing config_loader later.

SwarmConfig SwarmConfigLoader::load(const std::string& path) {
    SwarmConfig cfg;

    std::ifstream in(path);
    if (!in.is_open()) {
        // If config is missing, we just return empty swarm.
        return cfg;
    }

    std::string line;
    ProverConfig current;
    bool inProverBlock = false;

    while (std::getline(in, line)) {
        // Trim spaces
        auto trim = [](std::string& s) {
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
            size_t i = 0;
            while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
            s = s.substr(i);
        };
        trim(line);

        if (line.empty() || line[0] == '#') continue;

        if (line == "[[provers]]") {
            if (inProverBlock) {
                cfg.provers.push_back(current);
                current = ProverConfig{};
            }
            inProverBlock = true;
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);
        trim(key);
        trim(value);

        // Strip quotes if present
        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "pubkey") {
            current.pubkey = value;
        } else if (key == "capacity") {
            current.capacity = std::stod(value);
        } else if (key == "latency_ms") {
            current.latency_ms = std::stod(value);
        } else if (key == "reputation") {
            current.reputation = std::stod(value);
        }
    }

    if (inProverBlock) {
        cfg.provers.push_back(current);
    }

    return cfg;
}
