"""
Replay Router
Exposes replay mode tick-level introspection and audit-trail endpoints
"""

import logging
from typing import List, Optional
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field
from api.l2_client import get_ailee_client

router = APIRouter()
logger = logging.getLogger(__name__)


class EnvelopeModel(BaseModel):
    id: int
    payload_hash: str
    status: str
    timestamp: int
    type: str


class BitcoinClockStateModel(BaseModel):
    consensus_time: float
    height: int
    interval_seconds: float


class ReplayEventModel(BaseModel):
    block_hash: str
    height: int
    txid: str
    type: int


class ClusterViewModel(BaseModel):
    clock: BitcoinClockStateModel
    coherence: float
    envelopes: List[EnvelopeModel]
    node_count: int
    replay_events: List[ReplayEventModel]


class ReplayTickResponse(BaseModel):
    cluster_view: ClusterViewModel
    replay_mode_state: str
    replay_phase: str
    scheduler_state_hash: str
    telemetry_json: str
    tick_index: int


@router.get("/replay/tick/{index}", response_model=ReplayTickResponse)
async def get_replay_tick(index: int):
    """
    Get Deterministic Replay Tick by Index
    Proxies to C++ node to query tick-level introspection
    """
    client = get_ailee_client()
    cpp_tick = await client.get_replay_tick_by_index(index)

    if cpp_tick:
        return cpp_tick

    # Standalone mock fallback
    return {
        "cluster_view": {
            "clock": {
                "consensus_time": 1000.0,
                "height": 10,
                "interval_seconds": 600.0
            },
            "coherence": 100.00,
            "envelopes": [
                {
                    "id": 1,
                    "payload_hash": "mock_hash",
                    "status": "delivered",
                    "timestamp": 1005,
                    "type": "gossip"
                }
            ],
            "node_count": 4,
            "replay_events": []
        },
        "replay_mode_state": "active",
        "replay_phase": "execution",
        "scheduler_state_hash": "mock_scheduler_state",
        "telemetry_json": "{}",
        "tick_index": index
    }


@router.get("/replay/audit", response_model=List[ReplayTickResponse])
async def get_replay_audit():
    """
    Get Structured Replay Audit Trail
    Proxies to C++ node to query file-based audit trail
    """
    client = get_ailee_client()
    cpp_audit = await client.get_replay_audit()

    if cpp_audit is not None:
        return cpp_audit

    return []
