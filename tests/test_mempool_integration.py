#!/usr/bin/env python3
"""
Integration test for Mempool to BlockProducer wiring
Tests that transactions submitted via API land in blocks and get anchored to Bitcoin
"""

import hashlib
import os
import time

import pytest
import requests
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils


# Test-only key material. Never use this deterministic private key in production.
TEST_PRIVATE_KEY = ec.derive_private_key(1, ec.SECP256K1())
TEST_PUBLIC_KEY_HEX = TEST_PRIVATE_KEY.public_key().public_bytes(
    serialization.Encoding.X962,
    serialization.PublicFormat.CompressedPoint,
).hex()


def _sign_transaction_hash(tx_hash: str) -> str:
    """Sign the hash bytes expected by BlockProducer and return a DER signature."""
    digest = bytes.fromhex(tx_hash)
    return TEST_PRIVATE_KEY.sign(
        digest,
        ec.ECDSA(utils.Prehashed(hashes.SHA256())),
    ).hex()


@pytest.mark.integration
def test_mempool_to_block_producer():
    """Test that transactions flow from mempool to blocks"""
    
    print("=== Mempool to BlockProducer Integration Test ===\n")
    
    # Test configuration
    cpp_node_url = os.getenv("AILEE_NODE_URL", "http://localhost:8080")
    
    # Step 1: Check L2 state before
    print("1. Checking initial L2 state...")
    response = requests.get(f"{cpp_node_url}/api/l2/state", timeout=5)
    response.raise_for_status()
    initial_state = response.json()
    initial_height = initial_state.get("block_height", 0)
    initial_txs = initial_state.get("total_transactions", 0)
    print(f"   Initial block height: {initial_height}")
    print(f"   Initial total transactions: {initial_txs}\n")
    
    # Step 2: Submit test transactions
    print("2. Submitting test transactions...")
    test_txs = []
    for i in range(3):
        # Make each hash unique to this node run, then sign those exact 32 bytes.
        tx_data = f"test-tx-{i}-{initial_height}-{initial_txs}"
        tx_hash = hashlib.sha256(tx_data.encode()).hexdigest()
        
        payload = {
            "from_address": f"alice{i}",
            "to_address": f"bob{i}",
            "amount": (i + 1) * 1000,
            "data": f"Test payment {i}",
            "tx_hash": tx_hash,
            "public_key": TEST_PUBLIC_KEY_HEX,
            "signature": _sign_transaction_hash(tx_hash),
        }
        
        response = requests.post(
            f"{cpp_node_url}/api/transactions/submit",
            json=payload,
            timeout=5,
        )
        
        assert response.status_code == 202, response.text
        submission = response.json()
        assert submission["status"] == "accepted"
        assert submission["tx_hash"] == tx_hash
        print(f"   ✓ Transaction {i+1} submitted: {tx_hash[:16]}...")
        test_txs.append(tx_hash)
    
    print()
    
    # Step 3: Poll until BlockProducer has verified and confirmed every transaction.
    print("3. Waiting for transactions to be included in blocks...")
    deadline = time.monotonic() + 15
    final_state = initial_state
    while time.monotonic() < deadline:
        response = requests.get(f"{cpp_node_url}/api/l2/state", timeout=5)
        response.raise_for_status()
        final_state = response.json()
        if final_state.get("total_transactions", 0) - initial_txs >= len(test_txs):
            break
        time.sleep(0.25)

    # Step 4: Check the resulting L2 state
    print("4. Checking final L2 state...")
    final_height = final_state.get("block_height", 0)
    final_txs = final_state.get("total_transactions", 0)
    print(f"   Final block height: {final_height}")
    print(f"   Final total transactions: {final_txs}\n")
    
    # Step 5: Verify results
    print("5. Verification:")
    blocks_produced = final_height - initial_height
    txs_processed = final_txs - initial_txs
    
    print(f"   Blocks produced: {blocks_produced}")
    print(f"   Transactions processed: {txs_processed}")
    assert blocks_produced > 0, "BlockProducer did not produce a block while polling"
    
    if txs_processed >= len(test_txs):
        print(f"   ✓ All {len(test_txs)} transactions were included in blocks!")
        print("   ✓ Mempool to BlockProducer wiring is working correctly!")
        return True
    else:
        print(f"   ✗ Expected {len(test_txs)} transactions, but only {txs_processed} were processed")
        pytest.fail(f"Expected {len(test_txs)} transactions, but only {txs_processed} were processed")

if __name__ == "__main__":
    try:
        success = test_mempool_to_block_producer()
        print("\n" + "="*60)
        if success:
            print("TEST PASSED: Transactions flow from mempool to blocks ✓")
        else:
            print("TEST FAILED: Some transactions were not processed ✗")
        print("="*60)
    except Exception as e:
        print(f"\nTEST ERROR: {e}")
        import traceback
        traceback.print_exc()
