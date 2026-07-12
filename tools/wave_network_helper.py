#!/usr/bin/env python3
"""
Wave Network Integration Helper for AILEE-Core V32.
Validates Wave Native Network configurations to ensure proper federation setup,
keys, and message format.
"""
import argparse
import json
import re
import sys
from urllib.parse import urlparse

def is_valid_hex_pubkey(pubkey):
    """Validates if pubkey is a 64-character (32-byte) hex string."""
    return bool(re.match(r'^[0-9a-fA-F]{64}$', pubkey))

def is_valid_url(url):
    """Basic URL validation for endpoints."""
    try:
        result = urlparse(url)
        return all([result.scheme, result.netloc])
    except ValueError:
        return False

def validate_wave_config(config_data):
    report = {
        "status": "valid",
        "errors": [],
        "federation_id": None,
        "node_count": 0
    }

    if not isinstance(config_data, dict):
        report["status"] = "invalid"
        report["errors"].append("Configuration must be a JSON object.")
        return report

    fed_id = config_data.get("federation_id")
    if not fed_id or not isinstance(fed_id, str):
        report["status"] = "invalid"
        report["errors"].append("Missing or invalid 'federation_id'.")
    else:
        report["federation_id"] = fed_id

    # Check version
    if "message_format_version" not in config_data:
        report["status"] = "invalid"
        report["errors"].append("Missing 'message_format_version'.")

    nodes = config_data.get("nodes")
    if not isinstance(nodes, list) or len(nodes) == 0:
        report["status"] = "invalid"
        report["errors"].append("Configuration must contain a non-empty 'nodes' array.")
    else:
        report["node_count"] = len(nodes)
        seen_ids = set()
        seen_pubkeys = set()

        for i, node in enumerate(nodes):
            node_id = node.get("id")
            pubkey = node.get("pubkey")
            endpoint = node.get("endpoint")

            if not node_id:
                report["status"] = "invalid"
                report["errors"].append(f"Node at index {i} is missing 'id'.")
            elif node_id in seen_ids:
                report["status"] = "invalid"
                report["errors"].append(f"Duplicate node id found: {node_id}.")
            else:
                seen_ids.add(node_id)

            if not pubkey:
                report["status"] = "invalid"
                report["errors"].append(f"Node {node_id or i} is missing 'pubkey'.")
            elif not is_valid_hex_pubkey(pubkey):
                report["status"] = "invalid"
                report["errors"].append(f"Node {node_id or i} has an invalid pubkey (must be 64-char hex string).")
            elif pubkey in seen_pubkeys:
                report["status"] = "invalid"
                report["errors"].append(f"Duplicate pubkey found on node {node_id or i}.")
            else:
                seen_pubkeys.add(pubkey)

            if not endpoint:
                report["status"] = "invalid"
                report["errors"].append(f"Node {node_id or i} is missing 'endpoint'.")
            elif not is_valid_url(endpoint):
                report["status"] = "invalid"
                report["errors"].append(f"Node {node_id or i} has an invalid endpoint URL: {endpoint}.")

    return report

def main():
    parser = argparse.ArgumentParser(description="AILEE-Core Wave Network Integration Helper")
    parser.add_argument("--config", type=str, required=True, help="Path to the Wave Native Network JSON configuration file")
    parser.add_argument("--out", type=str, help="Optional output JSON file path", default=None)

    args = parser.parse_args()

    try:
        with open(args.config, "r") as f:
            config_data = json.load(f)
    except FileNotFoundError:
        report = {
            "status": "error",
            "errors": [f"Configuration file not found: {args.config}"]
        }
    except json.JSONDecodeError as e:
        report = {
            "status": "error",
            "errors": [f"Invalid JSON format in {args.config}: {e}"]
        }
    else:
        report = validate_wave_config(config_data)

    output_json = json.dumps(report, indent=2)

    if args.out:
        with open(args.out, "w") as f:
            f.write(output_json)
    else:
        print(output_json)

if __name__ == "__main__":
    main()
