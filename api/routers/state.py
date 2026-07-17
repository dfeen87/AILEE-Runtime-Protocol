"""
State Router
Exposes structured snapshots of deterministic key runtime state
"""

import logging
from typing import List
from fastapi import APIRouter
from pydantic import BaseModel, Field
from api.l2_client import get_ailee_client

router = APIRouter()
logger = logging.getLogger(__name__)


class ActiveSessionModel(BaseModel):
    session_id: str
    status: str


class BroadcastQueueModel(BaseModel):
    pending_count: int
    queue_id: str


class StateSnapshotResponse(BaseModel):
    active_sessions: List[ActiveSessionModel]
    broadcast_queues: List[BroadcastQueueModel]
    current_tick_index: int
    global_tick_count: int
    replay_active: bool
    subsystem_tick_count: int
    total_ticks: int


@router.get("/state/snapshot", response_model=StateSnapshotResponse)
async def get_state_snapshot():
    """
    Get Deterministic State Projection Snapshot
    Proxies to C++ node to query the state-projection module
    """
    client = get_ailee_client()
    cpp_snapshot = await client.get_state_snapshot()

    if cpp_snapshot:
        return cpp_snapshot

    # Standalone mock fallback
    return {
        "active_sessions": [
            {"session_id": "session_1", "status": "idle"},
            {"session_id": "session_2", "status": "active"}
        ],
        "broadcast_queues": [
            {"pending_count": 5, "queue_id": "mempool_queue"},
            {"pending_count": 1, "queue_id": "anchor_queue"}
        ],
        "current_tick_index": 0,
        "global_tick_count": 0,
        "replay_active": False,
        "subsystem_tick_count": 0,
        "total_ticks": 100
    }
