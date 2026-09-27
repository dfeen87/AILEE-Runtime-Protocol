"""
AILEE Trust Layer v37.1.0 API Router.
Provides REST endpoints for posture evaluation, governance approval gate, ALCOA ledger,
compartment state machine, and regime interpretation.
"""

import hashlib
import time
from typing import List, Optional
from fastapi import APIRouter, HTTPException, Query, Security
from pydantic import BaseModel, Field

router = APIRouter(prefix="/v37", tags=["v37-protocol"])


# Pydantic Request & Response Models

class PostureEvaluationRequest(BaseModel):
    current_fee_rate: float = Field(25.0, ge=0.0)
    high_fee_band: float = Field(50.0, ge=0.0)
    recent_volatility: float = Field(0.2, ge=0.0, le=1.0)
    block_interval_avg: int = Field(600, ge=0)
    time_since_last_block: int = Field(300, ge=0)
    daily_change_pct: float = Field(1.0)
    signal_coherence: float = Field(0.95, ge=0.0, le=1.0)


class PostureResponse(BaseModel):
    risk_score: float
    regime: str
    regime_id: str
    summary: str
    confidence: float
    temporal_coherence_index: float


class ApprovalFactorsRequest(BaseModel):
    quorum_count: int = Field(4, ge=0)
    total_validators: int = Field(5, ge=1)
    operator_signature_valid: bool = True
    system_signature_valid: bool = True
    zk_state_consistent: bool = True
    posture_score: float = Field(1.5, ge=0.0)
    temporal_coherence_index: float = Field(0.95, ge=0.0, le=1.0)
    zk_recursion_root: str = "0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0"


class GateDecisionResponse(BaseModel):
    approved: bool
    quorum_passed: bool
    signatures_valid: bool
    zk_valid: bool
    posture_passed: bool
    coherence_passed: bool
    rejection_reason: Optional[str] = None
    evaluated_factors: List[str]


class AlcoaEntryModel(BaseModel):
    entry_id: str
    operator_id: str
    system_id: str
    operator_signature: str
    human_readable_summary: str
    regime_label: str
    compartment_label: str
    timestamp_utc: int
    epoch_id: int
    epoch_hash: str
    parent_entry_id: Optional[str] = None
    source_system: str
    posture_regime_id: str
    posture_score: float
    zk_recursion_root: str
    temporal_coherence_index: float
    signal_energy: float
    coherence_score: float


class CompartmentModel(BaseModel):
    compartment_id: str
    name: str
    state: str
    last_transition_timestamp: int
    isolation_policy: str


# In-Memory State for API Router (v37)
_MOCK_LEDGER: List[AlcoaEntryModel] = [
    AlcoaEntryModel(
        entry_id="alcoa-0x425e70c240a19594bac19c354a12fa7e",
        operator_id="operator-canonical",
        system_id="ailee-v37-core",
        operator_signature="0x616c636f61736967",
        human_readable_summary="Genesis entry for AILEE-Trust-Layer v37.1.0",
        regime_label="neutral",
        compartment_label="core-execution",
        timestamp_utc=1743000000,
        epoch_id=3700,
        epoch_hash="0x425e70c240a19594bac19c354a12fa7edbe6e6c6d3c5790e32b5af52de6ff8ee",
        parent_entry_id="0x0000000000000000000000000000000000000000000000000000000000000000",
        source_system="AILEE-Trust-Layer-37.1.0",
        posture_regime_id="neutral",
        posture_score=1.5,
        zk_recursion_root="0xf4ffb4ab1eb4e31906c42f16367af8b10664f91876d1c932e1c44f96c0821c0f",
        temporal_coherence_index=0.98,
        signal_energy=25.0,
        coherence_score=0.98
    )
]

_MOCK_COMPARTMENTS: List[CompartmentModel] = [
    CompartmentModel(
        compartment_id="core-execution",
        name="Core Execution Engine",
        state="ACTIVE",
        last_transition_timestamp=1743000000,
        isolation_policy="strict-deterministic"
    ),
    CompartmentModel(
        compartment_id="governance-gate",
        name="Multi-Factor Governance Approval Gate",
        state="ACTIVE",
        last_transition_timestamp=1743000000,
        isolation_policy="multi-sig-quorum"
    ),
    CompartmentModel(
        compartment_id="alcoa-ledger",
        name="ALCOA Immutable Ledger Subsystem",
        state="ACTIVE",
        last_transition_timestamp=1743000000,
        isolation_policy="append-only-hash-chain"
    ),
    CompartmentModel(
        compartment_id="network-relay",
        name="Wave Native Network Relay",
        state="MONITORED",
        last_transition_timestamp=1743000000,
        isolation_policy="phase-coherence-bound"
    )
]


# Endpoint Implementations

@router.post("/posture/evaluate", response_model=PostureResponse)
async def evaluate_posture(req: PostureEvaluationRequest):
    risk_score = 0.0
    high_band = max(1.0, req.high_fee_band)
    if req.current_fee_rate > high_band * 1.5:
        risk_score += 3.0
    elif req.current_fee_rate > high_band:
        risk_score += 1.5

    if req.recent_volatility > 0.8:
        risk_score += 3.0
    elif req.recent_volatility > 0.5:
        risk_score += 1.5

    if abs(req.block_interval_avg - 600) > 300:
        risk_score += 2.0
    if req.time_since_last_block > 1800:
        risk_score += 2.0

    risk_score = min(10.0, max(0.0, risk_score))

    regime_id = "neutral"
    if req.recent_volatility > 0.7 and req.daily_change_pct > 5.0:
        regime_id = "parabolic"
    elif req.recent_volatility > 0.7 and req.daily_change_pct < -5.0:
        regime_id = "risk-off"
    elif req.recent_volatility < 0.3:
        regime_id = "chop"
    elif risk_score >= 7.0:
        regime_id = "stress"
    elif risk_score < 3.0 and req.signal_coherence > 0.8:
        regime_id = "recovery"

    summary = "Network conditions stable with high temporal coherence." if risk_score < 3.0 else "Elevated network fee/volatility risk."

    return PostureResponse(
        risk_score=risk_score,
        regime=regime_id.upper(),
        regime_id=regime_id,
        summary=summary,
        confidence=1.0 if req.time_since_last_block <= 3600 else 0.5,
        temporal_coherence_index=max(0.0, min(1.0, req.signal_coherence))
    )


@router.post("/governance/gate/evaluate", response_model=GateDecisionResponse)
async def evaluate_approval_gate(factors: ApprovalFactorsRequest):
    quorum_passed = (factors.quorum_count >= 3 and
                      factors.total_validators >= 3 and
                      factors.quorum_count <= factors.total_validators)
    signatures_valid = factors.operator_signature_valid and factors.system_signature_valid

    root = factors.zk_recursion_root.strip()
    root_valid = bool(root and root != "0x0" and root != "0x" + "0" * 64)
    zk_valid = factors.zk_state_consistent and root_valid

    posture_passed = factors.posture_score <= 2.5
    coherence_passed = factors.temporal_coherence_index >= 0.70

    approved = quorum_passed and signatures_valid and zk_valid and posture_passed and coherence_passed

    rejection_reason = None
    if not approved:
        if not quorum_passed:
            rejection_reason = "Quorum threshold not met"
        elif not signatures_valid:
            rejection_reason = "Invalid operator or system signatures"
        elif not zk_valid:
            rejection_reason = "ZK proof state inconsistent or stale recursion root"
        elif not posture_passed:
            rejection_reason = "Posture risk score exceeds safety thresholds"
        elif not coherence_passed:
            rejection_reason = "Temporal coherence index below threshold"

    evaluated_factors = [
        f"Quorum: {factors.quorum_count}/{factors.total_validators} (min: 3) -> {'PASS' if quorum_passed else 'FAIL'}",
        f"Signatures: {'PASS' if signatures_valid else 'FAIL'}",
        f"ZK State: {'PASS' if zk_valid else 'FAIL'}",
        f"Posture: {factors.posture_score} <= 2.5 -> {'PASS' if posture_passed else 'FAIL'}",
        f"Coherence: {factors.temporal_coherence_index} >= 0.70 -> {'PASS' if coherence_passed else 'FAIL'}"
    ]

    return GateDecisionResponse(
        approved=approved,
        quorum_passed=quorum_passed,
        signatures_valid=signatures_valid,
        zk_valid=zk_valid,
        posture_passed=posture_passed,
        coherence_passed=coherence_passed,
        rejection_reason=rejection_reason,
        evaluated_factors=evaluated_factors
    )


@router.get("/ledger/entries", response_model=List[AlcoaEntryModel])
async def get_ledger_entries(limit: int = Query(50, ge=1, le=1000)):
    return _MOCK_LEDGER[-limit:]


@router.get("/compartments", response_model=List[CompartmentModel])
async def get_compartments():
    return _MOCK_COMPARTMENTS


@router.get("/status")
async def get_v37_status():
    return {
        "version": "37.1.0",
        "status": "OPERATIONAL",
        "protocol": "AILEE-Trust-Layer-37.1.0",
        "multi_factor_governance": True,
        "alcoa_ledger_active": True,
        "compartments_count": len(_MOCK_COMPARTMENTS)
    }
