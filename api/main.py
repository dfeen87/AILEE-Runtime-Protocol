"""
AILEE-Core REST API - Main Application
Bitcoin Layer-2 Trust Oracle
Production-ready FastAPI implementation with deterministic, safe, read-only endpoints
"""

import asyncio
import logging
import os
import platform
import psutil
import sys
import time
from contextlib import asynccontextmanager
from datetime import datetime, timezone

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse, FileResponse
from fastapi.staticfiles import StaticFiles
import httpx
from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded

from api.config import settings
from api.routers import health, status, trust, l2, metrics, transactions, state, replay, energy
from api.security_audit import get_audit_logger, audit_log, AuditEventType, AuditEventSeverity
from api.database import init_database, close_database
from api.auth import init_api_key


# Configure logging
logging.basicConfig(
    level=getattr(logging, settings.log_level.upper()),
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    stream=sys.stdout
)
logger = logging.getLogger(__name__)

# Initialize security audit logger
audit_logger = get_audit_logger(log_file=settings.audit_log_path)

# Global state
startup_time = None


async def _keepalive_loop(interval: int) -> None:
    """
    Background keepalive task.

    Periodically pings the server's own health endpoint so that PaaS platforms
    (Fly.io, Railway, etc.) do not consider the process idle and suspend or stop
    the machine.  Any connection or HTTP errors are caught and logged at DEBUG
    level so they never disrupt normal operation.

    Args:
        interval: Seconds to sleep between each ping.
    """
    await asyncio.sleep(interval)  # initial delay – let startup complete first
    while True:
        try:
            async with httpx.AsyncClient(timeout=5.0) as client:
                response = await client.get(
                    f"http://127.0.0.1:{settings.port}/health"
                )
            logger.info(
                "💓 Keepalive ping – status %s", response.status_code
            )
        except (httpx.RequestError, httpx.HTTPStatusError) as exc:
            logger.debug("💓 Keepalive ping failed (non-critical): %s", exc)
        await asyncio.sleep(interval)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    Application lifespan manager
    Handles startup and shutdown events with deterministic initialization
    """
    global startup_time
    
    # Startup
    logger.info("=" * 80)
    logger.info("AILEE-Core REST API")
    logger.info("Bitcoin Layer-2 Trust Oracle")
    logger.info(f"Version: {settings.app_version}")
    logger.info(f"Environment: {settings.env}")
    logger.info(f"Node ID: {settings.node_id}")
    logger.info(f"System: {platform.platform()}")
    logger.info(f"Python: {sys.version.split()[0]}")
    logger.info("=" * 80)
    logger.info("")
    logger.info("🚀 Starting AILEE-Core API Server...")
    logger.info(f"📍 Binding to {settings.host}:{settings.port}")
    logger.info(f"   └─ PORT env var: {os.getenv('PORT', 'not set')} | AILEE_PORT env var: {os.getenv('AILEE_PORT', 'not set')}")
    logger.info(f"🔒 JWT Auth: {'Enabled' if settings.jwt_enabled else 'Disabled'}")
    logger.info(f"🌐 CORS: {'Enabled' if settings.cors_enabled else 'Disabled'}")
    logger.info(f"⏱️  Rate Limiting: {'Enabled' if settings.rate_limit_enabled else 'Disabled'}")
    
    if settings.rate_limit_enabled:
        logger.info(f"   └─ Limit: {settings.rate_limit_requests} requests per {settings.rate_limit_window}s")
    
    # Initialize database
    logger.info("")
    logger.info("💾 Initializing database...")
    logger.info(f"   └─ Database path: {settings.db_path}")
    try:
        await init_database(settings.db_path)
        logger.info("✅ Database initialized successfully")
    except Exception as e:
        logger.error(f"❌ Failed to initialize database: {e}", exc_info=True)
        raise
    
    # Initialize API key
    logger.info("")
    logger.info("🔑 Initializing API authentication...")
    try:
        api_key = init_api_key()
        logger.info("✅ API authentication initialized")
    except Exception as e:
        logger.error(f"❌ Failed to initialize API key: {e}", exc_info=True)
        raise
    
    # Check C++ node connection
    from api.l2_client import get_ailee_client
    client = get_ailee_client()
    cpp_node_url = client.base_url
    
    logger.info("")
    logger.info("🔗 C++ AILEE-Core Node Configuration:")
    logger.info(f"   └─ Endpoint URL: {cpp_node_url}")
    logger.info(f"   └─ Source: {'AILEE_NODE_URL env var' if os.getenv('AILEE_NODE_URL') else 'default (localhost:8080)'}")
    logger.info(f"   └─ Timeout: {client.timeout}s")
    
    logger.info("")
    logger.info("🔍 Testing C++ node connectivity...")
    is_healthy = await client.health_check()
    
    logger.info("")
    if is_healthy:
        logger.info("✅ C++ AILEE-Core Node Status: CONNECTED")
        logger.info("   └─ Health check: PASSED")
        logger.info("   └─ Real L2 data: AVAILABLE")
    else:
        logger.warning("⚠️  C++ AILEE-Core Node Status: NOT AVAILABLE")
        logger.warning(f"   └─ Could not connect to: {cpp_node_url}")
        logger.warning("   └─ API will run in standalone mode (mock data only)")
        logger.warning("   └─ To connect to C++ node:")
        logger.warning(f"      • Set AILEE_NODE_URL env var to correct endpoint")
        logger.warning(f"      • Ensure C++ node is running and accessible")
        logger.warning(f"      • Test with: curl {cpp_node_url}/api/health")
    
    logger.info("")
    logger.info("✅ Deterministic initialization complete")
    logger.info("✅ Configuration loaded and validated")
    logger.info("✅ All endpoints registered")
    logger.info("")
    logger.info("🎯 API is ready to serve requests")
    logger.info("📚 OpenAPI documentation available at: /docs")
    logger.info("🔍 Alternative docs available at: /redoc")
    logger.info("")
    
    startup_time = time.time()

    # Start keepalive background task
    keepalive_task = None
    if settings.keepalive_enabled:
        logger.info(
            f"💓 Starting keepalive task (interval: {settings.keepalive_interval_seconds}s)"
        )
        keepalive_task = asyncio.create_task(
            _keepalive_loop(settings.keepalive_interval_seconds)
        )

    yield

    # Shutdown
    logger.info("")
    logger.info("🛑 Shutting down AILEE-Core API Server...")

    # Cancel keepalive task
    if keepalive_task is not None:
        keepalive_task.cancel()
        try:
            await keepalive_task
        except asyncio.CancelledError:
            pass
    
    # Close database
    try:
        await close_database()
        logger.info("✅ Database closed successfully")
    except Exception as e:
        logger.error(f"❌ Failed to close database: {e}", exc_info=True)
    
    # Close C++ node client
    from api.l2_client import close_ailee_client
    await close_ailee_client()
    
    uptime = time.time() - startup_time
    logger.info(f"⏱️  Total uptime: {uptime:.2f} seconds")
    
    # Log memory usage at shutdown
    try:
        process = psutil.Process()
        mem_info = process.memory_info()
        logger.info(f"🧠 Memory at shutdown: {mem_info.rss / 1024 / 1024:.2f} MB")
    except Exception:
        pass
        
    logger.info("✅ Graceful shutdown complete")
    logger.info("=" * 80)


# Initialize rate limiter (if enabled)
limiter = None
if settings.rate_limit_enabled:
    limiter = Limiter(key_func=get_remote_address)


# Create FastAPI application
app = FastAPI(
    title=settings.app_name,
    description=settings.app_description,
    version=settings.app_version,
    lifespan=lifespan,
    docs_url="/docs",
    redoc_url="/redoc",
    openapi_url="/openapi.json"
)


# Add rate limiting middleware
if settings.rate_limit_enabled and limiter:
    app.state.limiter = limiter
    app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)


# Add CORS middleware
if settings.cors_enabled:
    app.add_middleware(
        CORSMiddleware,
        allow_origins=settings.cors_origins,
        allow_credentials=True,
        allow_methods=["GET", "POST", "OPTIONS"],
        allow_headers=["*"],
    )


# Global exception handler
@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """
    Global exception handler for unhandled errors
    Returns safe, deterministic error responses
    """
    logger.error(f"Unhandled exception: {exc}", exc_info=True)
    
    return JSONResponse(
        status_code=500,
        content={
            "error": "internal_server_error",
            "message": "An internal error occurred",
            "timestamp": datetime.now(timezone.utc).isoformat()
        }
    )


# Request logging middleware
@app.middleware("http")
async def log_requests(request: Request, call_next):
    """
    Log all incoming requests with timing information
    """
    start_time = time.time()
    
    # Log request
    logger.debug(f"→ {request.method} {request.url.path}")
    
    # Process request
    response = await call_next(request)
    
    # Log response
    duration = (time.time() - start_time) * 1000  # Convert to ms
    logger.debug(f"← {request.method} {request.url.path} - {response.status_code} ({duration:.2f}ms)")
    
    return response


# Include routers
app.include_router(health.router, tags=["Health"])
app.include_router(status.router, tags=["Status"])
app.include_router(trust.router, prefix="/trust", tags=["Trust"])
app.include_router(l2.router, prefix="/l2", tags=["Layer-2"])
app.include_router(transactions.router, prefix="/transactions", tags=["Transactions"])
app.include_router(metrics.router, tags=["Metrics"])
app.include_router(state.router, tags=["State"])
app.include_router(replay.router, tags=["Replay"])
app.include_router(energy.router, tags=["Energy"])

# Mount static files (web dashboard)
# Serve index.html at root, other static files at /web
import os
web_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "web")
if os.path.exists(web_dir):
    app.mount("/web", StaticFiles(directory=web_dir), name="web")


# Root endpoint
@app.get("/", include_in_schema=False)
async def root():
    """
    Root endpoint - Serve web dashboard if available, otherwise return API info
    """
    import os
    web_index = os.path.join(os.path.dirname(os.path.dirname(__file__)), "web", "index.html")
    if os.path.exists(web_index):
        return FileResponse(web_index)
    
    # Fallback to JSON API info if web dashboard not available
    return {
        "name": settings.app_name,
        "version": settings.app_version,
        "description": settings.app_description,
        "node_id": settings.node_id,
        "environment": settings.env,
        "documentation": {
            "openapi": "/openapi.json",
            "swagger": "/docs",
            "redoc": "/redoc"
        },
        "endpoints": {
            "health": "/health",
            "status": "/status",
            "trust_score": "/trust/score",
            "trust_validate": "/trust/validate",
            "l2_state": "/l2/state",
            "l2_anchors": "/l2/anchors",
            "transactions_submit": "/transactions/submit",
            "transactions_list": "/transactions/list",
            "metrics": "/metrics"
        },
        "timestamp": datetime.now(timezone.utc).isoformat()
    }


@app.get("/api", include_in_schema=True)
async def api_info():
    """
    API information endpoint
    
    Returns:
        Basic information about the AILEE-Core REST API
    """
    return {
        "name": settings.app_name,
        "version": settings.app_version,
        "description": settings.app_description,
        "node_id": settings.node_id,
        "environment": settings.env,
        "documentation": {
            "openapi": "/openapi.json",
            "swagger": "/docs",
            "redoc": "/redoc"
        },
        "endpoints": {
            "health": "/health",
            "status": "/status",
            "trust_score": "/trust/score",
            "trust_validate": "/trust/validate",
            "l2_state": "/l2/state",
            "l2_anchors": "/l2/anchors",
            "transactions_submit": "/transactions/submit",
            "transactions_list": "/transactions/list",
            "metrics": "/metrics"
        },
        "timestamp": datetime.now(timezone.utc).isoformat()
    }


# Get startup time for status endpoint
def get_startup_time():
    """Get the startup time of the application"""
    return startup_time
