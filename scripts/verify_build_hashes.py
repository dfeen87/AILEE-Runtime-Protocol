import argparse
import hashlib
import json
import os
import sys

def hash_file(path):
    if not os.path.exists(path):
        return None
    hasher = hashlib.sha256()
    with open(path, 'rb') as f:
        while chunk := f.read(8192):
            hasher.update(chunk)
    return hasher.hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--metadata', required=True)
    parser.add_argument('--node')
    parser.add_argument('--service')
    parser.add_argument('--rust-prover')
    args = parser.parse_args()

    with open(args.metadata, "r") as f:
        metadata = json.load(f)

    errors = []

    if args.node and os.path.exists(args.node):
        expected = metadata.get("node_hash", "")
        actual = hash_file(args.node)
        if actual != expected:
            errors.append(f"Node hash mismatch: expected {expected}, got {actual}")

    if args.service and os.path.exists(args.service):
        expected = metadata.get("service_hash", "")
        actual = hash_file(args.service)
        if actual != expected:
            errors.append(f"Service hash mismatch: expected {expected}, got {actual}")

    if args.rust_prover and os.path.exists(args.rust_prover):
        expected = metadata.get("rust_prover_hash", "")
        actual = hash_file(args.rust_prover)
        if actual != expected:
            errors.append(f"Rust prover hash mismatch: expected {expected}, got {actual}")

    if errors:
        for err in errors:
            print(f"ERROR: {err}", file=sys.stderr)
        sys.exit(1)

    print("Build hashes verified successfully.")

if __name__ == "__main__":
    main()
