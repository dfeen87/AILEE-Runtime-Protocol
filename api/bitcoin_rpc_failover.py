"""
Bitcoin RPC Failover Manager
Handles automatic failover between multiple Bitcoin RPC endpoints
Critical for production Layer-2 deployments
"""

import logging
import time
from typing import List, Optional, Dict, Any
from dataclasses import dataclass
from enum import Enum
import requests
from requests.auth import HTTPBasicAuth
import threading
from datetime import datetime, timedelta

logger = logging.getLogger(__name__)


class EndpointStatus(Enum):
    """Bitcoin RPC endpoint health status"""
    HEALTHY = "healthy"
    DEGRADED = "degraded"
    UNHEALTHY = "unhealthy"
    UNKNOWN = "unknown"


@dataclass
class BitcoinRPCEndpoint:
    """Bitcoin RPC endpoint configuration"""
    url: str
    username: str
    password: str
    priority: int = 0  # Lower number = higher priority
    name: str = ""
    use_tls: bool = True
    verify_tls: bool = True
    timeout: int = 30
    
    # Health tracking
    status: EndpointStatus = EndpointStatus.UNKNOWN
    last_success: Optional[datetime] = None
    last_failure: Optional[datetime] = None
    consecutive_failures: int = 0
    total_requests: int = 0
    successful_requests: int = 0
    avg_latency_ms: float = 0.0


class BitcoinRPCFailoverManager:
    """
    Manages failover between multiple Bitcoin RPC endpoints
    
    Features:
    - Automatic failover on endpoint failure
    - Health checking and monitoring
    - Circuit breaker pattern
    - Request retries with exponential backoff
    - Latency tracking
    """
    
    def __init__(
        self,
        endpoints: List[BitcoinRPCEndpoint],
        health_check_interval: int = 30,
        max_retries: int = 3,
        circuit_breaker_threshold: int = 5,
        circuit_breaker_timeout: int = 60
    ):
        """
        Initialize failover manager
        
        Args:
            endpoints: List of Bitcoin RPC endpoints
            health_check_interval: Seconds between health checks
            max_retries: Maximum retry attempts per endpoint
            circuit_breaker_threshold: Failures before circuit opens
            circuit_breaker_timeout: Seconds before retry on open circuit
        """
        if not endpoints:
            raise ValueError("At least one Bitcoin RPC endpoint required")
        if max_retries < 1:
            raise ValueError("max_retries must be at least 1")
        if circuit_breaker_threshold < 1:
            raise ValueError("circuit_breaker_threshold must be at least 1")
        
        self.endpoints = sorted(endpoints, key=lambda x: x.priority)
        self.health_check_interval = health_check_interval
        self.max_retries = max_retries
        self.circuit_breaker_threshold = circuit_breaker_threshold
        self.circuit_breaker_timeout = circuit_breaker_timeout
        
        self._lock = threading.Lock()
        self._health_check_thread = None
        self._running = False
        
        # Circuit breaker state
        self._circuit_open_until: Dict[str, datetime] = {}
        
        logger.info(
            f"Initialized Bitcoin RPC failover manager with {len(endpoints)} endpoints"
        )
    
    def start(self):
        """Start background health checking"""
        if self._running:
            logger.warning("Failover manager already running")
            return
        
        self._running = True
        self._health_check_thread = threading.Thread(
            target=self._health_check_loop,
            daemon=True
        )
        self._health_check_thread.start()
        logger.info("Started Bitcoin RPC health checking")
    
    def stop(self):
        """Stop background health checking"""
        self._running = False
        if self._health_check_thread:
            self._health_check_thread.join(timeout=5)
        logger.info("Stopped Bitcoin RPC health checking")
    
    def _health_check_loop(self):
        """Background health check loop"""
        while self._running:
            try:
                self._perform_health_checks()
            except Exception as e:
                logger.error(f"Health check error: {e}", exc_info=True)
            
            time.sleep(self.health_check_interval)
    
    def _perform_health_checks(self):
        """Check health of all endpoints"""
        for endpoint in self.endpoints:
            try:
                # Simple health check: getblockcount
                start_time = time.time()
                result = self._rpc_call(endpoint, "getblockcount", [], retry=False)
                latency = (time.time() - start_time) * 1000
                
                if result is not None:
                    with self._lock:
                        endpoint.status = EndpointStatus.HEALTHY
                        endpoint.last_success = datetime.now()
                        endpoint.consecutive_failures = 0
                        self._circuit_open_until.pop(endpoint.url, None)
                        # Update rolling average latency
                        if endpoint.avg_latency_ms == 0:
                            endpoint.avg_latency_ms = latency
                        else:
                            endpoint.avg_latency_ms = (
                                endpoint.avg_latency_ms * 0.9 + latency * 0.1
                            )
                    
                    logger.debug(
                        f"Health check OK: {endpoint.name or endpoint.url} "
                        f"(latency: {latency:.2f}ms)"
                    )
                else:
                    self._mark_endpoint_unhealthy(endpoint)
            
            except Exception as e:
                logger.warning(
                    f"Health check failed for {endpoint.name or endpoint.url}: {e}"
                )
                self._mark_endpoint_unhealthy(endpoint)
    
    def _mark_endpoint_unhealthy(self, endpoint: BitcoinRPCEndpoint):
        """Mark endpoint as unhealthy"""
        with self._lock:
            endpoint.consecutive_failures += 1
            endpoint.last_failure = datetime.now()
            
            if endpoint.consecutive_failures >= self.circuit_breaker_threshold:
                endpoint.status = EndpointStatus.UNHEALTHY
                # Open circuit breaker
                self._circuit_open_until[endpoint.url] = (
                    datetime.now() + timedelta(seconds=self.circuit_breaker_timeout)
                )
                logger.error(
                    f"Circuit breaker opened for {endpoint.name or endpoint.url} "
                    f"(failures: {endpoint.consecutive_failures})"
                )
            else:
                endpoint.status = EndpointStatus.DEGRADED
    
    def _is_circuit_open(self, endpoint: BitcoinRPCEndpoint) -> bool:
        """Check if circuit breaker is open for endpoint"""
        if endpoint.url not in self._circuit_open_until:
            return False
        
        if datetime.now() < self._circuit_open_until[endpoint.url]:
            return True
        
        # Circuit breaker timeout expired, remove from dict
        del self._circuit_open_until[endpoint.url]
        return False
    
    def rpc_call(self, method: str, params: List[Any] = None) -> Any:
        """
        Execute Bitcoin RPC call with automatic failover
        
        Args:
            method: RPC method name
            params: RPC method parameters
        
        Returns:
            RPC call result
        
        Raises:
            Exception: If all endpoints fail
        """
        params = params or []
        last_exception = None
        
        # Try each endpoint in priority order
        for endpoint in self.endpoints:
            # Skip if circuit breaker is open
            if self._is_circuit_open(endpoint):
                logger.debug(
                    f"Skipping {endpoint.name or endpoint.url} (circuit open)"
                )
                continue
            
            # Skip unhealthy endpoints (but try if it's the last one)
            if endpoint.status == EndpointStatus.UNHEALTHY:
                if endpoint != self.endpoints[-1]:
                    logger.debug(
                        f"Skipping {endpoint.name or endpoint.url} (unhealthy)"
                    )
                    continue
            
            try:
                result = self._rpc_call(endpoint, method, params, retry=True)
                
                # Success!
                with self._lock:
                    endpoint.successful_requests += 1
                    endpoint.total_requests += 1
                    endpoint.last_success = datetime.now()
                    endpoint.consecutive_failures = 0
                    
                    if endpoint.status != EndpointStatus.HEALTHY:
                        endpoint.status = EndpointStatus.HEALTHY
                        logger.info(
                            f"Endpoint recovered: {endpoint.name or endpoint.url}"
                        )
                
                return result
            
            except Exception as e:
                last_exception = e
                logger.warning(
                    f"RPC call failed on {endpoint.name or endpoint.url}: {e}"
                )
                
                with self._lock:
                    endpoint.total_requests += 1

                # Record this failed call exactly once and update circuit state.
                self._mark_endpoint_unhealthy(endpoint)
                
                # Continue to next endpoint
                continue
        
        # All endpoints failed
        logger.error(f"All Bitcoin RPC endpoints failed for method: {method}")
        raise Exception(
            f"All Bitcoin RPC endpoints failed. Last error: {last_exception}"
        )
    
    def _rpc_call(
        self,
        endpoint: BitcoinRPCEndpoint,
        method: str,
        params: List[Any],
        retry: bool = True
    ) -> Any:
        """
        Execute RPC call on specific endpoint
        
        Args:
            endpoint: Target endpoint
            method: RPC method
            params: RPC parameters
            retry: Whether to retry on failure
        
        Returns:
            RPC result
        """
        payload = {
            "jsonrpc": "2.0",
            "id": "ailee-l2",
            "method": method,
            "params": params
        }
        
        auth = HTTPBasicAuth(endpoint.username, endpoint.password)
        
        attempts = self.max_retries if retry else 1
        
        for attempt in range(attempts):
            try:
                response = requests.post(
                    endpoint.url,
                    json=payload,
                    auth=auth,
                    timeout=endpoint.timeout,
                    verify=endpoint.verify_tls if endpoint.use_tls else False
                )
                
                response.raise_for_status()
                
                result = response.json()
                
                if "error" in result and result["error"] is not None:
                    raise Exception(f"RPC error: {result['error']}")
                
                return result.get("result")
            
            except Exception as e:
                if attempt < attempts - 1:
                    # Exponential backoff with jitter
                    wait_time = (2 ** attempt) + (time.time() % 1)
                    logger.debug(
                        f"Retry {attempt + 1}/{attempts} after {wait_time:.2f}s: {e}"
                    )
                    time.sleep(wait_time)
                else:
                    raise
    
    def get_status(self) -> Dict[str, Any]:
        """Get current status of all endpoints"""
        with self._lock:
            return {
                "endpoints": [
                    {
                        "name": ep.name or ep.url,
                        "url": ep.url,
                        "priority": ep.priority,
                        "status": ep.status.value,
                        "last_success": ep.last_success.isoformat() if ep.last_success else None,
                        "last_failure": ep.last_failure.isoformat() if ep.last_failure else None,
                        "consecutive_failures": ep.consecutive_failures,
                        "success_rate": (
                            ep.successful_requests / ep.total_requests * 100
                            if ep.total_requests > 0 else 0
                        ),
                        "avg_latency_ms": ep.avg_latency_ms,
                        "circuit_open": self._is_circuit_open(ep)
                    }
                    for ep in self.endpoints
                ]
            }


# Example usage
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    # Configure multiple Bitcoin RPC endpoints
    endpoints = [
        BitcoinRPCEndpoint(
            url="https://primary.bitcoin.local:8332",
            username="bitcoinrpc",
            password="secret1",
            priority=0,
            name="Primary Node"
        ),
        BitcoinRPCEndpoint(
            url="https://backup1.bitcoin.local:8332",
            username="bitcoinrpc",
            password="secret2",
            priority=1,
            name="Backup Node 1"
        ),
        BitcoinRPCEndpoint(
            url="https://backup2.bitcoin.local:8332",
            username="bitcoinrpc",
            password="secret3",
            priority=2,
            name="Backup Node 2"
        ),
    ]
    
    # Initialize failover manager
    manager = BitcoinRPCFailoverManager(endpoints)
    manager.start()
    
    try:
        # Make RPC calls - will automatically failover on errors
        block_count = manager.rpc_call("getblockcount")
        print(f"Block count: {block_count}")
        
        # Get status
        status = manager.get_status()
        print(f"Status: {status}")
    
    finally:
        manager.stop()
