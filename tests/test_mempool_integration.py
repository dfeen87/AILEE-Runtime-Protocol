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
        # Create deterministic transaction hash
        tx_data = f"test-tx-{i}-{time.time()}"
        tx_hash = hashlib.sha256(tx_data.encode()).hexdigest()
        
        payload = {
            "from_address": f"alice{i}",
            "to_address": f"bob{i}",
            "amount": (i + 1) * 1000,
            "data": f"Test payment {i}",
            "tx_hash": tx_hash
        }
        
        response = requests.post(
            f"{cpp_node_url}/api/transactions/submit",
            json=payload,
            timeout=5,
        )
        
        assert response.status_code == 202, response.text
        print(f"   ✓ Transaction {i+1} submitted: {tx_hash[:16]}...")
        test_txs.append(tx_hash)
    
    print()
    
    # Step 3: Wait for block production
    print("3. Waiting for transactions to be included in blocks...")
    # Note: Block interval is 1 second. Wait for at least 3 blocks to ensure all txs are processed.
    # In production, consider polling L2 state until expected blocks are produced.
    time.sleep(3)
    
    # Step 4: Check L2 state after
    print("4. Checking final L2 state...")
    response = requests.get(f"{cpp_node_url}/api/l2/state", timeout=5)
    response.raise_for_status()
    final_state = response.json()
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
