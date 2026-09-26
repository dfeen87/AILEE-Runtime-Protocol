"""
AILEE Trust Layer v36.x Legacy API Compatibility Router.
Provides backward-compatible REST endpoints for posture evaluation, governance gate, ALCOA ledger,
and status while mapping requests internally to v37 core logic safely.
"""

from typing import List, Optional
from fastapi import APIRouter, Query
from pydantic import BaseModel, Field

from api.routers.v37 import (
    PostureEvaluationRequest,
    PostureResponse,
    ApprovalFactorsRequest,
    GateDecisionResponse,
    AlcoaEntryModel,
    CompartmentModel,
    evaluate_posture,
    evaluate_approval_gate,
    get_ledger_entries,
    get_compartments,
)

router = APIRouter(prefix="/v36", tags=["v36-legacy-compatibility"])


# V36 Response Models (Backward-compatible schema)

class V36StatusResponse(BaseModel):
    version: str = "36.5.0"
    status: str = "OPERATIONAL"
    protocol: str = "AILEE-Trust-Layer-36.5.0-compatible"
    v37_bridge_active: bool = True


@router.post("/posture/evaluate", response_model=PostureResponse)
async def evaluate_posture_v36(req: PostureEvaluationRequest):
    """
    V36 backward-compatible posture evaluation endpoint.
    Delegates to v37 posture evaluation core.
    """
    return await evaluate_posture(req)


@router.post("/governance/gate/evaluate", response_model=GateDecisionResponse)
async def evaluate_approval_gate_v36(factors: ApprovalFactorsRequest):
    """
    V36 backward-compatible approval gate endpoint.
    Delegates to v37 governance gate core.
    """
    return await evaluate_approval_gate(factors)


@router.get("/ledger/entries", response_model=List[AlcoaEntryModel])
async def get_ledger_entries_v36(limit: int = Query(50, ge=1, le=1000)):
    """
    V36 backward-compatible ALCOA ledger entries endpoint.
    """
    return await get_ledger_entries(limit)


@router.get("/compartments", response_model=List[CompartmentModel])
async def get_compartments_v36():
    """
    V36 backward-compatible compartments endpoint.
    """
    return await get_compartments()


@router.get("/status", response_model=V36StatusResponse)
async def get_v36_status():
    """
    V36 version status endpoint.
    """
    return V36StatusResponse()
