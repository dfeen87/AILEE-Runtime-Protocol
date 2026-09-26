import time
import pytest
from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)

def test_api_approval_gate_stress():
    payload = {
        "quorum_count": 4,
        "total_validators": 5,
        "operator_signature_valid": True,
        "system_signature_valid": True,
        "zk_state_consistent": True,
        "posture_score": 1.5,
        "temporal_coherence_index": 0.95,
        "zk_recursion_root": "0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0"
    }

    start = time.time()
    iterations = 500
    for _ in range(iterations):
        response = client.post("/v37/governance/gate/evaluate", json=payload)
        assert response.status_code == 200
        assert response.json()["approved"] is True

    elapsed = time.time() - start
    assert elapsed < 10.0 # Must complete 500 requests in < 10 seconds

def test_api_posture_evaluate_stress():
    payload = {
        "current_fee_rate": 25.0,
        "high_fee_band": 50.0,
        "recent_volatility": 0.2,
        "block_interval_avg": 600,
        "time_since_last_block": 300,
        "daily_change_pct": 1.0,
        "signal_coherence": 0.95
    }

    start = time.time()
    iterations = 500
    for i in range(iterations):
        payload["current_fee_rate"] = 10.0 + (i % 50)
        response = client.post("/v37/posture/evaluate", json=payload)
        assert response.status_code == 200

    elapsed = time.time() - start
    assert elapsed < 10.0
