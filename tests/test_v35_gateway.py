import pytest
from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)


def test_state_snapshot_endpoint():
    """Test state snapshot endpoint exposes correct V35 attributes"""
    response = client.get("/state/snapshot")
    assert response.status_code == 200

    data = response.json()
    assert "active_sessions" in data
    assert "broadcast_queues" in data
    assert "current_tick_index" in data
    assert "global_tick_count" in data
    assert "replay_active" in data
    assert "subsystem_tick_count" in data
    assert "total_ticks" in data

    # Verify normalization/types
    assert isinstance(data["active_sessions"], list)
    for session in data["active_sessions"]:
        assert "session_id" in session
        assert "status" in session
        # No platform specific pointers
        assert not session["session_id"].startswith("0x")

    assert isinstance(data["broadcast_queues"], list)
    for queue in data["broadcast_queues"]:
        assert "pending_count" in queue
        assert "queue_id" in queue


def test_replay_tick_introspection():
    """Test replay tick schema is correctly extended under V35"""
    response = client.get("/replay/tick/42")
    assert response.status_code == 200

    data = response.json()
    assert data["tick_index"] == 42
    assert "replay_phase" in data
    assert "replay_mode_state" in data
    assert "cluster_view" in data
    assert "scheduler_state_hash" in data
    assert "telemetry_json" in data

    # Schema is additive and backward-compatible
    view = data["cluster_view"]
    assert "clock" in view
    assert "coherence" in view
    assert "envelopes" in view
    assert "node_count" in view
    assert "replay_events" in view


def test_replay_audit_endpoint():
    """Test replay audit trail endpoint works"""
    response = client.get("/replay/audit")
    assert response.status_code == 200
    assert isinstance(response.json(), list)


def test_energy_advisory_endpoint():
    """Test the newly hardened energy advisory endpoint with strict schemas"""
    # 1. Valid payload
    payload = {
        "voltage": 400.0,
        "current": 150.0,
        "temperature": 35.0,
        "soc": 0.8,
        "dt": 1.0
    }
    response = client.post("/energy/advisory", json=payload)
    assert response.status_code == 200
    data = response.json()
    assert "torque_scale" in data
    assert "risk_score" in data
    assert "confidence" in data
    assert data["require_derating"] is False

    # 2. Extreme but valid inputs triggering derating
    payload_extreme = {
        "voltage": 450.0,
        "current": 450.0, # triggers derating
        "temperature": 55.0, # triggers derating
        "soc": 0.4,
        "dt": 1.0
    }
    response = client.post("/energy/advisory", json=payload_extreme)
    assert response.status_code == 200
    data_ext = response.json()
    assert data_ext["require_derating"] is True
    assert data_ext["torque_scale"] == 0.5
    assert data_ext["confidence"] == 0.6

    # 3. Invalid inputs rejected by strict Pydantic models (SoC out of range)
    payload_invalid = {
        "voltage": 400.0,
        "current": 100.0,
        "temperature": 25.0,
        "soc": 1.2, # Out of range [0.0, 1.0]
        "dt": 1.0
    }
    response = client.post("/energy/advisory", json=payload_invalid)
    assert response.status_code == 422 # Validation error
