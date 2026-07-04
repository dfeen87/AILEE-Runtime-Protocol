import argparse
import hashlib
import json
import os

def hash_file(path):
    if not os.path.exists(path):
        return ""
    hasher = hashlib.sha256()
    with open(path, 'rb') as f:
        while chunk := f.read(8192):
            hasher.update(chunk)
    return hasher.hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out-json', required=True)
    parser.add_argument('--out-hpp', required=True)
    parser.add_argument('--node-bin')
    parser.add_argument('--service-bin')
    parser.add_argument('--rust-prover-lib')
    args = parser.parse_args()

    metadata = {
        "build_id": os.environ.get("BUILD_ID", "local"),
    }

    if args.node_bin:
        metadata["node_hash"] = hash_file(args.node_bin)
    if args.service_bin:
        metadata["service_hash"] = hash_file(args.service_bin)
    if args.rust_prover_lib:
        metadata["rust_prover_hash"] = hash_file(args.rust_prover_lib)

    with open(args.out_json, "w") as f:
        json.dump(metadata, f, indent=4)

    with open(args.out_hpp, "w") as f:
        f.write("#pragma once\n\n")
        f.write(f'#define AILEE_BUILD_ID "{metadata.get("build_id", "")}"\n')
        f.write(f'#define AILEE_RUST_PROVER_HASH "{metadata.get("rust_prover_hash", "")}"\n')

if __name__ == "__main__":
    main()
