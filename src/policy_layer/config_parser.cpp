#include "ailee_api.hpp"

// We will use nlohmann json
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace ailee {

AileeConfig load_config(const std::string& filepath) {
    AileeConfig config;
    config.environment = "simulation";
    config.strict_mode = true;

    try {
        std::ifstream f(filepath);
        if (f.is_open()) {
            std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            json data = json::parse(str);
            if (data.contains("environment")) config.environment = data["environment"].get<std::string>();
            if (data.contains("strict_mode")) config.strict_mode = data["strict_mode"].get<bool>();
        } else {
            std::cerr << "Could not open config file: " << filepath << " - using defaults." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file " << filepath << ": " << e.what() << std::endl;
    }

    return config;
}

Policy load_policy(const std::string& filepath) {
    Policy policy;
    // Default safe policy
    policy.max_acceptable_risk_score = 5.0;
    policy.max_fee_rate_sats_per_vbyte = 100.0;
    policy.min_confidence_required = 0.8;
    policy.allowed_regimes = {"neutral", "risk-on"};
    policy.block_interval_tolerance_sec = 300.0;

    try {
        std::ifstream f(filepath);
        if (f.is_open()) {
            std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            json data = json::parse(str);
            if (data.contains("max_acceptable_risk_score"))
                policy.max_acceptable_risk_score = data["max_acceptable_risk_score"].get<double>();
            if (data.contains("max_fee_rate_sats_per_vbyte"))
                policy.max_fee_rate_sats_per_vbyte = data["max_fee_rate_sats_per_vbyte"].get<double>();
            if (data.contains("min_confidence_required"))
                policy.min_confidence_required = data["min_confidence_required"].get<double>();
            if (data.contains("allowed_regimes")) {
                policy.allowed_regimes.clear();
                for (const auto& item : data["allowed_regimes"]) {
                    policy.allowed_regimes.push_back(item.get<std::string>());
                }
            }
            if (data.contains("block_interval_tolerance_sec"))
                policy.block_interval_tolerance_sec = data["block_interval_tolerance_sec"].get<double>();
        } else {
             std::cerr << "Could not open policy file: " << filepath << " - using defaults." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing policy file " << filepath << ": " << e.what() << std::endl;
    }

    return policy;
}

} // namespace ailee
