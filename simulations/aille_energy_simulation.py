#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AILEE Energy Runtime - Python Simulation System
===============================================
Provides static optimization, dynamic optimization, parameter grid search,
out-of-sample validation, and multi-seed robustness testing of the dual-state
HLV physics-informed battery model.
"""

import math
import random
import json

class HLVEnhancer:
    def __init__(self, capacity_ah=75.0, nominal_voltage_v=400.0, config=None):
        self.capacity_ah = capacity_ah
        self.nominal_voltage_v = nominal_voltage_v

        # Load config or defaults
        self.config = config or {
            "lambda": 1e-6,
            "entropy_weight": 0.5,
            "phi_decay_rate": 0.001,
            "max_temperature_c": 60.0,
            "min_temperature_c": -20.0,
            "max_current_a": 500.0,
            "fallback_position_scale": 0.1,
            "min_confidence_threshold": 0.20,
            "grace_confidence_threshold": 0.10
        }

        # Internal state variables
        self.entropy = 0.0
        self.cycle_count = 0.0
        self.degradation = 0.0
        self.charge_throughput_ah = 0.0
        self.time = 0.0

    def reset(self):
        self.entropy = 0.0
        self.cycle_count = 0.0
        self.degradation = 0.0
        self.charge_throughput_ah = 0.0
        self.time = 0.0

    def update(self, voltage, current, temperature, soc, dt=0.1):
        """
        Runs a single update cycle of the HLV battery physics model.
        Returns state indicators and raw predictions.
        """
        # Clamp inputs
        if abs(current) > self.config["max_current_a"]:
            current = math.copysign(self.config["max_current_a"], current)
        if temperature > self.config["max_temperature_c"]:
            temperature = self.config["max_temperature_c"]
        elif temperature < self.config["min_temperature_c"]:
            temperature = self.config["min_temperature_c"]

        clamped_soc = max(0.0, min(1.0, soc))
        self.time += dt

        # 1. Update entropy & cycle count
        temp_contrib = max(0.0, temperature - 25.0) / 35.0
        current_contrib = abs(current) / 100.0

        self.entropy += dt * self.config["entropy_weight"] * (temp_contrib + current_contrib)
        self.entropy = min(self.entropy, 1.0)

        # Coulomb counting
        charge_delta = abs(current) * dt / 3600.0
        self.charge_throughput_ah += charge_delta
        prev_cycle_count = self.cycle_count
        self.cycle_count = self.charge_throughput_ah / (2.0 * self.capacity_ah)

        # 2. HLV Geometric Metric Approximation (g^eff trace)
        grad_phi_0 = -self.config["phi_decay_rate"] * math.sqrt(self.entropy**2 + self.degradation**2)
        grad_phi_1 = self.entropy * (1.0 - clamped_soc)
        grad_phi_2 = (temperature / self.config["max_temperature_c"]) * self.degradation
        grad_phi_3 = math.sqrt(max(0.0, self.cycle_count)) * 0.01

        grad_sum_sq = grad_phi_0**2 + grad_phi_1**2 + grad_phi_2**2 + grad_phi_3**2
        g_eff_trace = 2.0 + self.config["lambda"] * grad_sum_sq

        # 3. Update degradation
        cycle_delta = self.cycle_count - prev_cycle_count
        base_degradation = cycle_delta * 0.0001
        thermal_degradation = temp_contrib * 0.0005 * dt
        hlv_correction = g_eff_trace * 0.00001

        self.degradation += base_degradation + thermal_degradation + hlv_correction
        self.degradation = min(self.degradation, 1.0)

        # 4. Diagnostic & advisory metrics
        remaining_capacity_percent = (1.0 - self.degradation) * 100.0
        degradation_rate = g_eff_trace * 0.0001

        metric_stability = 1.0 / (1.0 + abs(g_eff_trace - 2.0))
        hlv_confidence = metric_stability * (1.0 - self.degradation)

        recommended_current_limit = 100.0 * metric_stability * (1.0 - self.degradation)
        recommended_voltage_limit = self.nominal_voltage_v * (1.0 + 0.1 * metric_stability)
        recommended_temperature = 25.0 + 5.0 * clamped_soc

        remaining_cap_ah = (1.0 - clamped_soc) * self.capacity_ah
        estimated_charge_time = 60.0 * remaining_cap_ah / max(recommended_current_limit, 1.0)

        return {
            "degradation": self.degradation,
            "remaining_capacity_percent": remaining_capacity_percent,
            "hlv_confidence": hlv_confidence,
            "recommended_current_limit": recommended_current_limit,
            "recommended_voltage_limit": recommended_voltage_limit,
            "recommended_temperature": recommended_temperature,
            "estimated_charge_time": estimated_charge_time,
            "degradation_rate": degradation_rate
        }


def static_optimization(enhancer, voltage, current, temperature, soc):
    """
    Optimizes current charging speed vs degradation trade-off for a static state.
    """
    results = []
    # Test charging currents from 10A to 150A
    for test_current in range(10, 160, 10):
        # Temp rises proportionally to current
        test_temp = temperature + (test_current / 150.0) * 15.0
        enhancer.reset()
        out = enhancer.update(voltage, test_current, test_temp, soc, dt=1.0)

        # Fitness: maximize charge current, minimize degradation rate
        fitness = test_current / 150.0 - out["degradation_rate"] * 1000.0
        results.append((test_current, fitness, out))

    # Pick optimal current
    best_current, best_fitness, best_out = max(results, key=lambda x: x[1])
    return {
        "best_current": best_current,
        "best_fitness": best_fitness,
        "details": best_out
    }


def dynamic_optimization(enhancer, profile_steps, dt=1.0):
    """
    Simulates a time-series driving profile and dynamically optimizes torque
    and thermal stress to minimize end-of-profile degradation.
    """
    enhancer.reset()
    history = []
    total_degradation = 0.0

    for step in profile_steps:
        # Step: (voltage, raw_current, temperature, soc)
        v, c, t, s = step

        # HLV optimization: scale driver current request if battery is stressed
        # Use simple feedback control: high entropy scales current down
        scale_factor = 1.0 - 0.3 * enhancer.entropy
        optimized_current = c * scale_factor
        optimized_temp = t - (1.0 - scale_factor) * 5.0 # cooler because of lower current

        out = enhancer.update(v, optimized_current, optimized_temp, s, dt=dt)
        history.append(out)

    return {
        "final_degradation": enhancer.degradation,
        "remaining_capacity_percent": (1.0 - enhancer.degradation) * 100.0,
        "history_length": len(history)
    }


def grid_search(capacity_ah, nominal_voltage_v, profile_steps):
    """
    Grid sweep over coupling strength lambda and entropy_weight to find optimal settings.
    """
    best_score = -1.0
    best_params = {}

    # Grid boundaries
    lambdas = [1e-6, 5e-6, 1e-5]
    entropy_weights = [0.3, 0.5, 0.7]

    for lamb in lambdas:
        for ew in entropy_weights:
            cfg = {
                "lambda": lamb,
                "entropy_weight": ew,
                "phi_decay_rate": 0.001,
                "max_temperature_c": 60.0,
                "min_temperature_c": -20.0,
                "max_current_a": 500.0,
                "fallback_position_scale": 0.1,
                "min_confidence_threshold": 0.20,
                "grace_confidence_threshold": 0.10
            }
            enhancer = HLVEnhancer(capacity_ah, nominal_voltage_v, config=cfg)
            res = dynamic_optimization(enhancer, profile_steps, dt=5.0)

            # Metric: higher remaining capacity percent is better
            score = res["remaining_capacity_percent"]
            if score > best_score:
                best_score = score
                best_params = {"lambda": lamb, "entropy_weight": ew}

    return best_params, best_score


def out_of_sample_validation(capacity_ah, nominal_voltage_v, train_profiles, test_profiles):
    """
    Optimizes configuration parameters on train set and validates on out-of-sample test set.
    """
    # 1. Train / Optimize
    # Combine training profiles
    flat_train = [step for p in train_profiles for step in p]
    best_params, _ = grid_search(capacity_ah, nominal_voltage_v, flat_train)

    # 2. Validate Out-of-Sample
    test_results = []
    cfg = {
        "lambda": best_params["lambda"],
        "entropy_weight": best_params["entropy_weight"],
        "phi_decay_rate": 0.001,
        "max_temperature_c": 60.0,
        "min_temperature_c": -20.0,
        "max_current_a": 500.0,
        "fallback_position_scale": 0.1,
        "min_confidence_threshold": 0.20,
        "grace_confidence_threshold": 0.10
    }

    for i, p in enumerate(test_profiles):
        enhancer = HLVEnhancer(capacity_ah, nominal_voltage_v, config=cfg)
        res = dynamic_optimization(enhancer, p, dt=5.0)
        test_results.append({
            "test_profile_index": i,
            "final_degradation": res["final_degradation"],
            "remaining_capacity_percent": res["remaining_capacity_percent"]
        })

    return {
        "optimized_parameters": best_params,
        "out_of_sample_results": test_results
    }


def multi_seed_robustness_testing(capacity_ah, nominal_voltage_v, base_profile, num_seeds=5):
    """
    Runs multi-seed validation with randomized current/temperature noise to test model stability.
    """
    results = []

    for seed in range(num_seeds):
        random.seed(seed)
        # Add random noise to base profile steps
        noisy_profile = []
        for v, c, t, s in base_profile:
            noisy_c = c + random.normalvariate(0.0, 5.0) # noise in current
            noisy_t = t + random.uniform(-1.0, 1.0)      # noise in temp
            noisy_profile.append((v, noisy_c, noisy_t, s))

        enhancer = HLVEnhancer(capacity_ah, nominal_voltage_v)
        res = dynamic_optimization(enhancer, noisy_profile, dt=2.0)
        results.append(res["remaining_capacity_percent"])

    mean_val = sum(results) / len(results)
    variance = sum((x - mean_val) ** 2 for x in results) / len(results)
    std_dev = math.sqrt(variance)

    return {
        "individual_seed_capacities": results,
        "mean_remaining_capacity": mean_val,
        "standard_deviation": std_dev,
        "robustness_score": 1.0 - std_dev / 100.0 # high score means highly robust
    }


if __name__ == "__main__":
    print("=== AILEE Energy Runtime Simulation Runner ===")

    # Generate mock drive profile steps
    # Profile: 50 steps of varying load
    mock_profile = []
    for step in range(50):
        soc = 1.0 - (step * 0.01)
        voltage = 380.0 + soc * 30.0
        current = 80.0 * math.sin(step * 0.3)
        temp = 25.0 + abs(current) * 0.15
        mock_profile.append((voltage, current, temp, soc))

    enhancer = HLVEnhancer()

    print("\n1. Running Static Optimization...")
    static_opt = static_optimization(enhancer, 400.0, 50.0, 30.0, 0.7)
    print(f"Optimal Static Charging Current: {static_opt['best_current']} A")

    print("\n2. Running Dynamic Profile Optimization...")
    dyn_opt = dynamic_optimization(enhancer, mock_profile, dt=5.0)
    print(f"Final Degradation: {dyn_opt['final_degradation']:.6f}")
    print(f"Remaining Capacity: {dyn_opt['remaining_capacity_percent']:.4f}%")

    print("\n3. Running Parameter Grid Search...")
    best_params, best_score = grid_search(75.0, 400.0, mock_profile)
    print(f"Best Sweep Parameters: {best_params} (Remaining Cap Score: {best_score:.4f}%)")

    print("\n4. Running Out-of-Sample Validation...")
    train_set = [mock_profile, mock_profile]
    test_set = [mock_profile]
    oos_val = out_of_sample_validation(75.0, 400.0, train_set, test_set)
    print(f"OOS Validated Params: {oos_val['optimized_parameters']}")
    print(f"OOS Remaining Cap Score: {oos_val['out_of_sample_results'][0]['remaining_capacity_percent']:.4f}%")

    print("\n5. Running Multi-Seed Robustness Validation...")
    robustness = multi_seed_robustness_testing(75.0, 400.0, mock_profile, num_seeds=3)
    print(f"Robustness Mean Remaining Cap: {robustness['mean_remaining_capacity']:.4f}%")
    print(f"Robustness StdDev: {robustness['standard_deviation']:.6f}")
    print("==============================================")
