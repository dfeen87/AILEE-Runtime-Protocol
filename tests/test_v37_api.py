import pytest
from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)

def test_v37_status():
    response = client.get("/v37/status")
    assert response.status_code == 200
    data = response.json()
    assert data["version"] == "37.0.0"
    assert data["status"] == "OPERATIONAL"
    assert data["multi_factor_governance"] is True

def test_v37_posture_evaluate():
    payload = {
        "current_fee_rate": 25.0,
        "high_fee_band": 50.0,
        "recent_volatility": 0.2,
        "block_interval_avg": 600,
        "time_since_last_block": 300,
        "daily_change_pct": 1.0,
        "signal_coherence": 0.95
    }
    response = client.post("/v37/posture/evaluate", json=payload)
    assert response.status_code == 200
    data = response.json()
    assert data["risk_score"] == 0.0
    assert data["regime"] == "CHOP"
    assert data["temporal_coherence_index"] == 0.95

def test_v37_approval_gate_evaluate():
    payload = {
        "quorum_count": 4,
        "total_validators": 5,
        "operator_signature_valid": True,
        "system_signature_valid": True,
        "zk_state_consistent": True,
        "posture_score": 1.5,
        "temporal_coherence_index": 0.95,
        "zk_recursion_root": "0x123456789abcdef"
    }
    response = client.post("/v37/governance/gate/evaluate", json=payload)
    assert response.status_code == 200
    data = response.json()
    assert data["approved"] is True
    assert data["quorum_passed"] is True
    assert len(data["evaluated_factors"]) == 5

def test_v37_ledger_and_compartments():
    resp_ledger = client.get("/v37/ledger/entries")
    assert resp_ledger.status_code == 200
    entries = resp_ledger.json()
    assert len(entries) >= 1
    assert entries[0]["epoch_id"] == 3700

    resp_comp = client.get("/v37/compartments")
    assert resp_comp.status_code == 200
    comps = resp_comp.json()
    assert len(comps) >= 4
