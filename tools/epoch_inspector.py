#!/usr/bin/env python3
"""
Epoch Replay Inspector for AILEE-Core V32.
Replays deterministic transitions for an epoch, compares expected vs actual state diffs,
and produces a deterministic verification report.
"""
import argparse
import json
import os
import subprocess
import sys

def load_expected_state_root(manifest_path, epoch):
    try:
        with open(manifest_path, "r") as f:
            manifest = json.load(f)
            return manifest.get("epoch_state_roots", {}).get(str(epoch))
    except FileNotFoundError:
        return None

def run_orchestrator(binary_path, epoch):
    try:
        result = subprocess.run(
            [binary_path, "--run-epoch", str(epoch)],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error running orchestrator: {e}", file=sys.stderr)
        return None
    except FileNotFoundError:
        print(f"Orchestrator binary not found: {binary_path}", file=sys.stderr)
        return None

def extract_state_root(stdout_str):
    # The JSON is always the final non-empty line of stdout
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
    parser = argparse.ArgumentParser(description="AILEE-Core Epoch Replay Inspector")
    parser.add_argument("--run-epoch", type=int, required=True, help="Epoch to run and inspect")
    parser.add_argument("--binary", type=str, default="./ailee_core_v12.bin", help="Path to the orchestrator binary")
    parser.add_argument("--manifest", type=str, default="v12_receipt_manifest.json", help="Path to the receipt manifest")
    parser.add_argument("--out", type=str, help="Optional output JSON file path", default=None)

    args = parser.parse_args()

    epoch = args.run_epoch

    expected_root = load_expected_state_root(args.manifest, epoch)
    if not expected_root:
        report = {
            "epoch": epoch,
            "status": "error",
            "message": f"Could not find expected state root for epoch {epoch} in {args.manifest}"
        }
    else:
        stdout_output = run_orchestrator(args.binary, epoch)
        if stdout_output is None:
            report = {
                "epoch": epoch,
                "status": "error",
                "message": "Failed to run orchestrator binary"
            }
        else:
            actual_root = extract_state_root(stdout_output)
            if not actual_root:
                report = {
                    "epoch": epoch,
                    "status": "error",
                    "message": "Failed to parse state root from orchestrator output"
                }
            else:
                report = {
                    "epoch": epoch,
                    "status": "valid" if actual_root == expected_root else "invalid",
                    "expected_state_root": expected_root,
                    "actual_state_root": actual_root,
                    "drift_detected": actual_root != expected_root
                }

    output_json = json.dumps(report, indent=2)

    if args.out:
        with open(args.out, "w") as f:
            f.write(output_json)
    else:
        print(output_json)

if __name__ == "__main__":
    main()
