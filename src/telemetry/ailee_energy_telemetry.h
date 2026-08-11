/**
 * AILEE Energy Telemetry Protocol — Enhanced Version
 * 
 * Cryptographic + thermodynamic verification layer for AILEE-Core.
 * Bridges physical waste-heat recovery hardware with blockchain-level proofs.
 * 
 * Additions:
 *  - Entropy-weighted efficiency computation (AILEE Canonical Method v1.4)
 *  - Environmental normalization (ambient temperature compensation)
 *  - Sensor anomaly detection & confidence scoring
 *  - Energy Integrity Score (EIS)
 *  - "GreenHash v2" (Merkle-ready hash for global proofs)
 *
 * Licensed under the MIT License
 * Copyright (c) 2026 Don Michael Feeney Jr.
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_ENERGY_TELEMETRY_H
#define AILEE_ENERGY_TELEMETRY_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <openssl/sha.h>

namespace ailee {

/**
 * Raw sensor data representing a thermal snapshot.
 */
struct ThermalMetric {
    double inputPowerWatts;
    double wasteHeatRecoveredW;
    double ambientTempC;
    double exhaustTempC;
    uint64_t timestamp;
};

/**
 * Extended analysis structure for AILEE-Oracles.
 */
struct EnergyAnalysis {
    double baseEfficiency;         // 0–1 classical efficiency
    double entropyCompensation;    // environmental weighting
    double stabilityFactor;        // 0–1 thermal stability
    double energyIntegrityScore;   // final AILEE score
    double sensorConfidence;       // anomaly detection
};

/**
 * AILEE Energy Telemetry
 * Canonical thermodynamic evaluation & cryptographic proof generation.
 */
class EnergyTelemetry {
public:

    // --- 1. Classical efficiency (baseline law-of-thermodynamics) ---
    static double calculateEfficiencyScore(const ThermalMetric& metric) {
        if (metric.inputPowerWatts <= 0) return 0.0;
        double eff = metric.wasteHeatRecoveredW / metric.inputPowerWatts;
        return std::clamp(eff, 0.0, 1.0);
    }

    // --- 2. Entropy compensation (cooler ambient → bonus, hotter → penalty) ---
    static double computeEntropyCompensation(double ambientC) {
        const double nominal = 22.0;
        double diff = std::fabs(ambientC - nominal);

        // Environmental entropy penalty curve (1 / (1 + diff * k))
        double k = 0.045;
        return 1.0 / (1.0 + diff * k);
    }

    // --- 3. Thermal stability (exhaust–ambient delta) ---
    static double computeThermalStability(double ambientC, double exhaustC) {
        if (exhaustC < ambientC) return 0.0; // physically invalid
        double delta = exhaustC - ambientC;

        // 0°C = unstable / 20°C ideal / >50°C degraded
        if (delta <= 5) return 0.15;
        if (delta >= 50) return 0.25;
        return 0.25 + (0.75 * (1.0 - (delta / 50.0))); // curves down with heat
    }

    // --- 4. Sensor anomaly detection ---
    static double computeSensorConfidence(const ThermalMetric& m) {
        if (m.inputPowerWatts < 50 || m.inputPowerWatts > 20000) return 0.1;
        if (m.ambientTempC < -10 || m.ambientTempC > 90) return 0.1;
        if (m.exhaustTempC < m.ambientTempC) return 0.0;

        double d = m.exhaustTempC - m.ambientTempC;
        if (d > 180) return 0.2;

        return 0.85 + (0.15 * (1 - (d / 180.0)));  // minor penalty for extreme heat
    }

    // --- 5. Unified Energy Integrity Score (EIS) ---
    static EnergyAnalysis analyze(const ThermalMetric& m) {
        EnergyAnalysis out{};

        out.baseEfficiency      = calculateEfficiencyScore(m);
        out.entropyCompensation = computeEntropyCompensation(m.ambientTempC);
        out.stabilityFactor     = computeThermalStability(
                                      m.ambientTempC, 
                                      m.exhaustTempC
                                  );
        out.sensorConfidence    = computeSensorConfidence(m);

        // AILEE Canonical Integration:
        // EIS = eff × entropy × stability × confidence
        out.energyIntegrityScore =
              out.baseEfficiency
            * out.entropyCompensation
            * out.stabilityFactor
            * out.sensorConfidence;

        out.energyIntegrityScore = std::clamp(out.energyIntegrityScore, 0.0, 1.0);
        return out;
    }

    // --- 6. GreenHash v2 — Merkle-ready cryptographic telemetry proof ---
    static std::string generateTelemetryProof(const ThermalMetric& metric,
                                              const std::string& nodeId)
    {
        EnergyAnalysis a = analyze(metric);

        std::string raw =
            nodeId +
            std::to_string(metric.inputPowerWatts) +
            std::to_string(metric.wasteHeatRecoveredW) +
            std::to_string(metric.ambientTempC) +
            std::to_string(metric.exhaustTempC) +
            std::to_string(a.energyIntegrityScore) +
            std::to_string(metric.timestamp);

        // SHA256
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*)raw.c_str(), raw.size(), hash);

        char hex[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            snprintf(hex + (i * 2), 3, "%02x", hash[i]);
        hex[SHA256_DIGEST_LENGTH * 2] = '\0';

        return std::string(hex);
    }
};

} // namespace ailee

#endif // AILEE_ENERGY_TELEMETRY_H
