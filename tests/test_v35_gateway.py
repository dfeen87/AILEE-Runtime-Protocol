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
