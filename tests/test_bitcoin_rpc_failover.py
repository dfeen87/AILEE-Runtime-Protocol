from unittest.mock import patch

import pytest

from api.bitcoin_rpc_failover import (
    BitcoinRPCEndpoint,
    BitcoinRPCFailoverManager,
    EndpointStatus,
)


def make_endpoint() -> BitcoinRPCEndpoint:
    return BitcoinRPCEndpoint(
        url="https://bitcoin.example.test",
        username="rpc-user",
        password="rpc-password",
    )


def test_failed_call_counts_once_and_opens_circuit_at_threshold():
    endpoint = make_endpoint()
    manager = BitcoinRPCFailoverManager(
        [endpoint], max_retries=1, circuit_breaker_threshold=2
    )

    with patch.object(manager, "_rpc_call", side_effect=RuntimeError("offline")):
        with pytest.raises(Exception, match="All Bitcoin RPC endpoints failed"):
            manager.rpc_call("getblockcount")

        assert endpoint.consecutive_failures == 1
        assert endpoint.status == EndpointStatus.DEGRADED

        with pytest.raises(Exception, match="All Bitcoin RPC endpoints failed"):
            manager.rpc_call("getblockcount")

    assert endpoint.consecutive_failures == 2
    assert endpoint.status == EndpointStatus.UNHEALTHY
    assert manager._is_circuit_open(endpoint)


@pytest.mark.parametrize("value", [0, -1])
def test_rejects_non_positive_retry_counts(value):
    with pytest.raises(ValueError, match="max_retries"):
        BitcoinRPCFailoverManager([make_endpoint()], max_retries=value)


def test_health_check_recovery_closes_circuit():
    endpoint = make_endpoint()
    manager = BitcoinRPCFailoverManager([endpoint], circuit_breaker_threshold=1)
    manager._mark_endpoint_unhealthy(endpoint)

    with patch.object(manager, "_rpc_call", return_value=840_000):
        manager._perform_health_checks()

    assert endpoint.status == EndpointStatus.HEALTHY
    assert endpoint.consecutive_failures == 0
    assert not manager._is_circuit_open(endpoint)
