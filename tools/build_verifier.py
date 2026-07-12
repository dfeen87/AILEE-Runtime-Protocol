#!/usr/bin/env python3
"""
Deterministic Build Verifier for AILEE-Core V32.
Recomputes build hashes, receipt hashes, validates frozen genesis, and outputs a verification report.
"""
import argparse
import hashlib
import json
import os
import sys

def calculate_sha256(filepath):
    """Calculates the SHA-256 hash of a file."""
    sha256_hash = hashlib.sha256()
    try:
        with open(filepath, "rb") as f:
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest()
    except FileNotFoundError:
        return None

def read_text_file(filepath):
    try:
        with open(filepath, "r") as f:
            return f.read().strip()
    except FileNotFoundError:
        return None

def load_json(filepath):
    try:
        with open(filepath, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        return None

def verify_hash(name, file_to_hash, expected_hash_file, manifest_embedded_hash=None):
    computed = calculate_sha256(file_to_hash)
    expected = read_text_file(expected_hash_file)

    result = {
        "component": name,
        "status": "valid" if computed == expected else "invalid",
        "computed_hash": computed,
        "expected_hash_file": expected,
    }

    if manifest_embedded_hash:
        result["manifest_embedded_hash"] = manifest_embedded_hash
        if computed != manifest_embedded_hash:
             result["status"] = "invalid (mismatch with manifest)"

    if not computed:
        result["status"] = "missing file"
        result["error"] = f"File {file_to_hash} not found"
    elif not expected:
         result["status"] = "missing expected hash file"
         result["error"] = f"File {expected_hash_file} not found"

    return result

def main():
    parser = argparse.ArgumentParser(description="AILEE-Core Deterministic Build Verifier")
    parser.add_argument("--out", type=str, help="Optional output JSON file path", default=None)
    parser.add_argument("--dir", type=str, help="Directory containing the artifacts (default: current directory)", default=".")

    args = parser.parse_args()

    report = {
        "version": "v12",
        "status": "valid",
        "checks": []
    }

    # The user specifically mentioned comparing to embedded hashes if they exist.
    reexecution_manifest = load_json(os.path.join(args.dir, "v12_reexecution_manifest.json")) or {}
    embedded_build_hash = reexecution_manifest.get("build_hash")
    embedded_receipt_hash = reexecution_manifest.get("receipt_hash")

    # 1. Build Hash Verification
    build_check = verify_hash(
        "Build Manifest",
        os.path.join(args.dir, "v12_build_manifest.json"),
        os.path.join(args.dir, "v12_build_hash.txt"),
        embedded_build_hash
    )
    report["checks"].append(build_check)

    # 2. Receipt Hash Verification
    receipt_check = verify_hash(
        "Receipt Manifest",
        os.path.join(args.dir, "v12_receipt_manifest.json"),
        os.path.join(args.dir, "v12_receipt_hash.txt"),
        embedded_receipt_hash
    )
    report["checks"].append(receipt_check)

    # 3. Reexecution Hash Verification
    reexec_check = verify_hash(
        "Reexecution Manifest",
        os.path.join(args.dir, "v12_reexecution_manifest.json"),
        os.path.join(args.dir, "v12_reexecution_hash.txt")
    )
    report["checks"].append(reexec_check)

    # 4. Frozen Genesis Verification
    genesis_file = os.path.join(args.dir, "v12_frozen_genesis.json")
    genesis_data = load_json(genesis_file)
    genesis_check = {
         "component": "Frozen Genesis",
         "status": "valid" if genesis_data and "genesis" in genesis_data else "invalid",
         "filepath": genesis_file
    }
    report["checks"].append(genesis_check)

    # Determine overall status
    for check in report["checks"]:
        if check.get("status") != "valid":
            report["status"] = "invalid"
            break

    output_json = json.dumps(report, indent=2)

    if args.out:
        with open(args.out, "w") as f:
            f.write(output_json)
    else:
        print(output_json)

if __name__ == "__main__":
    main()
