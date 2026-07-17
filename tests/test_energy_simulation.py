#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import unittest
import math
from simulations.aille_energy_simulation import (
    HLVEnhancer,
    static_optimization,
    dynamic_optimization,
    grid_search,
    out_of_sample_validation,
    multi_seed_robustness_testing
)

class TestEnergySimulation(unittest.TestCase):
    def setUp(self):
        self.capacity_ah = 75.0
        self.nominal_voltage_v = 400.0
        self.enhancer = HLVEnhancer(self.capacity_ah, self.nominal_voltage_v)

        # Build a base mock profile
        self.mock_profile = []
        for step in range(20):
            soc = 1.0 - (step * 0.02)
            voltage = 380.0 + soc * 30.0
            current = 50.0 * math.sin(step * 0.5)
            temp = 25.0 + abs(current) * 0.1
            self.mock_profile.append((voltage, current, temp, soc))

    def test_enhancer_initialization_and_update(self):
        self.assertEqual(self.enhancer.entropy, 0.0)
        self.assertEqual(self.enhancer.cycle_count, 0.0)
        self.assertEqual(self.enhancer.degradation, 0.0)

        # Single update cycle
        out = self.enhancer.update(390.0, 100.0, 35.0, 0.9, dt=5.0)
        self.assertGreater(out["degradation"], 0.0)
        self.assertGreater(out["hlv_confidence"], 0.0)
        self.assertLess(out["remaining_capacity_percent"], 100.0)

    def test_static_optimization(self):
        res = static_optimization(self.enhancer, 400.0, 50.0, 30.0, 0.7)
        self.assertIn("best_current", res)
        self.assertIn("best_fitness", res)
        self.assertTrue(10 <= res["best_current"] <= 150)

        # Assert optimization never violates safety bounds
        self.assertLessEqual(res["details"]["recommended_current_limit"], self.enhancer.config["max_current_a"])
        self.assertLessEqual(res["details"]["recommended_voltage_limit"], 500.0)

    def test_dynamic_optimization(self):
        res = dynamic_optimization(self.enhancer, self.mock_profile, dt=5.0)
        self.assertGreater(res["final_degradation"], 0.0)
        self.assertEqual(res["history_length"], 20)

        # Assert dynamic optimization never violates safety bounds
        self.assertLessEqual(self.enhancer.degradation, 1.0)
        self.assertGreaterEqual(self.enhancer.degradation, 0.0)

    def test_grid_search(self):
        best_params, best_score = grid_search(self.capacity_ah, self.nominal_voltage_v, self.mock_profile)
        self.assertIn("lambda", best_params)
        self.assertIn("entropy_weight", best_params)
        self.assertGreater(best_score, 0.0)

        # Assert grid search converges to stable recommendations where score doesn't swing wildly
        self.assertGreaterEqual(best_score, 99.0)
        self.assertLessEqual(best_score, 100.0)

    def test_out_of_sample_validation(self):
        train_set = [self.mock_profile, self.mock_profile]
        test_set = [self.mock_profile]
        res = out_of_sample_validation(self.capacity_ah, self.nominal_voltage_v, train_set, test_set)

        self.assertIn("optimized_parameters", res)
        self.assertIn("out_of_sample_results", res)
        self.assertEqual(len(res["out_of_sample_results"]), 1)

    def test_multi_seed_robustness(self):
        res = multi_seed_robustness_testing(self.capacity_ah, self.nominal_voltage_v, self.mock_profile, num_seeds=3)
        self.assertIn("individual_seed_capacities", res)
        self.assertEqual(len(res["individual_seed_capacities"]), 3)
        self.assertIn("standard_deviation", res)
        self.assertGreater(res["robustness_score"], 0.0)

        # Assert multi-seed runs produce bounded variance (standard deviation < 5% of mean)
        mean = res["mean_remaining_capacity"]
        std_dev = res["standard_deviation"]
        rel_std_dev = std_dev / mean if mean > 0 else 0.0
        self.assertLess(rel_std_dev, 0.05)

if __name__ == "__main__":
    unittest.main()
