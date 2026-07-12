#!/usr/bin/env python3
"""
State Root Comparison Tool for AILEE-Core V32.
Compares state roots across builds to detect nondeterministic drift.
Supports Mode A (comparing two JSON files) and Mode B (generating on the fly vs expected JSON).
"""
import argparse
import json
import os
import subprocess
import sys

def load_json(filepath):
    try:
        with open(filepath, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        return None

def extract_state_root_from_json(data):
    if not data:
        return None
    # Might be a single JSON object with state_root
    if "state_root" in data:
        return data["state_root"]
    return None

def get_state_root_from_file(filepath):
    data = load_json(filepath)
    return extract_state_root_from_json(data)

def run_orchestrator(binary_path, epoch):
    try:
        result = subprocess.run(
            [binary_path, "--run-epoch", str(epoch)],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except Exception as e:
        print(f"Error running orchestrator: {e}", file=sys.stderr)
        return None

def extract_state_root_from_stdout(stdout_str):
    lines = [line.strip() for line in stdout_str.split('\n') if line.strip()]
    if not lines:
        return None

    last_line = lines[-1]
    try:
        data = json.loads(last_line)
        return data.get("state_root")
    except json.JSONDecodeError:
        return None

def main():
    parser = argparse.ArgumentParser(description="AILEE-Core State Root Comparison Tool")

    # Mode A args
    parser.add_argument("--expected", type=str, help="Path to expected state root JSON file")
    parser.add_argument("--actual", type=str, help="Path to actual state root JSON file")

    # Mode B args
    parser.add_argument("--run-epoch", type=int, help="Epoch to run and compare")
    parser.add_argument("--compare-to", type=str, help="Path to canonical state root JSON file for comparison")
    parser.add_argument("--binary", type=str, default="./ailee_core_v12.bin", help="Path to orchestrator binary")

    # Common
    parser.add_argument("--out", type=str, help="Optional output JSON file path", default=None)

    args = parser.parse_args()

    report = {
        "status": "error",
        "message": "Invalid arguments."
    }

    mode_a = args.expected and args.actual
    mode_b = (args.run_epoch is not None) and args.compare_to

    if mode_a and mode_b:
        report["message"] = "Cannot use both Mode A and Mode B arguments simultaneously."
    elif mode_a:
        expected_root = get_state_root_from_file(args.expected)
        actual_root = get_state_root_from_file(args.actual)

        if not expected_root:
            report["message"] = f"Failed to extract state root from {args.expected}"
        elif not actual_root:
            report["message"] = f"Failed to extract state root from {args.actual}"
        else:
            report = {
                "mode": "file_comparison",
                "status": "match" if expected_root == actual_root else "mismatch",
                "expected_state_root": expected_root,
                "actual_state_root": actual_root,
                "drift_detected": expected_root != actual_root
            }
    elif mode_b:
        expected_root = get_state_root_from_file(args.compare_to)

        if not expected_root:
            report["message"] = f"Failed to extract state root from {args.compare_to}"
        else:
            stdout = run_orchestrator(args.binary, args.run_epoch)
            if stdout is None:
                report["message"] = "Failed to run orchestrator."
            else:
                actual_root = extract_state_root_from_stdout(stdout)
                if not actual_root:
                    report["message"] = "Failed to parse state root from orchestrator output."
                else:
                    report = {
                        "mode": "on_the_fly_generation",
                        "epoch": args.run_epoch,
                        "status": "match" if expected_root == actual_root else "mismatch",
                        "expected_state_root": expected_root,
                        "actual_state_root": actual_root,
                        "drift_detected": expected_root != actual_root
                    }
    else:
        report["message"] = "Provide either (--expected and --actual) OR (--run-epoch and --compare-to)."

    output_json = json.dumps(report, indent=2)

    if args.out:
        with open(args.out, "w") as f:
            f.write(output_json)
    else:
        print(output_json)

if __name__ == "__main__":
    main()
