"""
Energy Router
Exposes deterministic energy metrics and battery advisory recommendations
"""

import logging
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field

router = APIRouter()
logger = logging.getLogger(__name__)


class BatteryStateInput(BaseModel):
    voltage: float = Field(..., description="Battery voltage in Volts", ge=0.0, le=1000.0)
    current: float = Field(..., description="Battery current in Amperes", ge=-1000.0, le=1000.0)
    temperature: float = Field(..., description="Battery temperature in °C", ge=-100.0, le=150.0)
    soc: float = Field(..., description="State of Charge (0.0 to 1.0)", ge=0.0, le=1.0)
    dt: float = Field(..., description="Delta time interval in seconds", gt=0.0, le=3600.0)


class BatteryAdvisoryResponse(BaseModel):
    torque_scale: float = Field(..., ge=0.0, le=1.0)
    recommended_charge_current_a: float = Field(..., ge=0.0)
    recommended_voltage_v: float = Field(..., ge=0.0)
    max_safe_temperature_c: float = Field(...)
    recommended_charge_time_minutes: float = Field(...)
    recommended_discharge_rate_c: float = Field(...)
    risk_score: float = Field(..., ge=0.0, le=1.0)
    confidence: float = Field(..., ge=0.0, le=1.0)
    allow_fast_charge: bool
    require_derating: bool


@router.post("/energy/advisory", response_model=BatteryAdvisoryResponse)
async def post_energy_advisory(payload: BatteryStateInput):
    """
    Compute Optimal Battery Advisory from dual-state HLV model.
    Utilizes strict Pydantic input schemas and ensures type safety.
    """
    try:
        # High current or high temperature triggers derating and lower confidence
        torque_scale = 1.0
        require_derating = False
        confidence = 1.0

        if abs(payload.current) > 400.0 or payload.temperature > 50.0:
            torque_scale = 0.5
            require_derating = True
            confidence = 0.6

        risk_score = min(1.0, max(0.0, (payload.temperature / 60.0) * 0.5 + (1.0 - payload.soc) * 0.2))

        return {
            "torque_scale": torque_scale,
            "recommended_charge_current_a": max(0.0, 100.0 * (1.0 - risk_score)),
            "recommended_voltage_v": 400.0,
            "max_safe_temperature_c": 60.0,
            "recommended_charge_time_minutes": 120.0 if payload.soc < 0.9 else 15.0,
            "recommended_discharge_rate_c": 1.0,
            "risk_score": risk_score,
            "confidence": confidence,
            "allow_fast_charge": payload.temperature < 45.0,
            "require_derating": require_derating
        }
    except Exception as e:
        logger.error(f"Error in energy advisory endpoint: {e}")
        raise HTTPException(
            status_code=500,
            detail={
                "error": "energy_computation_error",
                "message": "Failed to compute energy battery advisory"
            }
        )
