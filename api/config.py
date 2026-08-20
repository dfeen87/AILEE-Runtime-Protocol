"""
AILEE-Core API Configuration
Safe, deterministic configuration loading with validation
"""

import os
from typing import Optional
from pydantic import AliasChoices, Field, field_validator, model_validator
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """
    AILEE-Core API Settings
    All settings can be overridden via environment variables
    """
    
    # Node Identity
    node_id: str = Field(
        default="ailee-node-default",
        description="Unique identifier for this AILEE node"
    )
    
    # Environment
    env: str = Field(
        default="production",
        description="Deployment environment: production, staging, development"
    )
    
    # Logging
    log_level: str = Field(
        default="info",
        description="Logging level: debug, info, warning, error, critical"
    )
    audit_log_path: str = Field(
        default="/app/logs/security-audit.log",
        description="Path to security audit log file"
    )
    
    # API Server
    host: str = Field(
        default="0.0.0.0",
        description="Host to bind the API server"
    )
    port: int = Field(
        default=8080,
        validation_alias=AliasChoices('PORT', 'AILEE_PORT'),
        description="Port to bind the API server (reads PORT env var first, then AILEE_PORT, defaults to 8080)"
    )
    
    # CORS
    cors_enabled: bool = Field(
        default=True,
        description="Enable CORS for browser access"
    )
    cors_origins: list = Field(
        default=["*"],
        description="Allowed CORS origins"
    )
    
    # Authentication
    jwt_enabled: bool = Field(
        default=False,
        description="Enable JWT authentication (disabled by default)"
    )
    jwt_secret: Optional[str] = Field(
        default=None,
        description="JWT secret key for token signing"
    )
    jwt_algorithm: str = Field(
        default="HS256",
        description="JWT signing algorithm"
    )
    
    # Rate Limiting
    rate_limit_enabled: bool = Field(
        default=True,
        description="Enable lightweight rate limiting"
    )
    rate_limit_requests: int = Field(
        default=100,
        description="Max requests per window"
    )
    rate_limit_window: int = Field(
        default=60,
        description="Rate limit window in seconds"
    )
    
    # Application Metadata
    app_name: str = Field(
        default="AILEE-Core REST API",
        description="Application name"
    )
    app_version: str = Field(
        default="36.0.0",
        description="Application version"
    )
    app_description: str = Field(
        default="Bitcoin Layer-2 Trust Oracle with deterministic, safe, read-only endpoints",
        description="Application description"
    )
    
    # Database Configuration
    db_path: str = Field(
        default="/data/ailee.db",
        description="Path to SQLite database file"
    )
    
    # Bitcoin RPC Configuration
    bitcoin_rpc_url: str = Field(
        default="https://localhost:8332",
        description="Primary Bitcoin RPC endpoint URL"
    )
    bitcoin_rpc_user: Optional[str] = Field(
        default=None,
        description="Bitcoin RPC username (use secrets manager in production)"
    )
    bitcoin_rpc_password: Optional[str] = Field(
        default=None,
        description="Bitcoin RPC password (use secrets manager in production)"
    )
    bitcoin_rpc_use_tls: bool = Field(
        default=True,
        description="Use TLS for Bitcoin RPC connections"
    )
    bitcoin_rpc_verify_tls: bool = Field(
        default=True,
        description="Verify TLS certificates for Bitcoin RPC"
    )
    bitcoin_rpc_timeout: int = Field(
        default=30,
        description="Bitcoin RPC request timeout in seconds"
    )
    bitcoin_rpc_failover_urls: Optional[str] = Field(
        default=None,
        description="Comma-separated list of failover Bitcoin RPC URLs"
    )
    
    # Keepalive Configuration
    keepalive_enabled: bool = Field(
        default=True,
        description="Enable background keepalive task to prevent idle platform suspension"
    )
    keepalive_interval_seconds: int = Field(
        default=60,
        gt=30,
        description="Interval in seconds between keepalive pings (default: 60s, minimum: 31s)"
    )

    # TLS/SSL Configuration
    tls_enabled: bool = Field(
        default=False,
        description="Enable TLS/SSL for the API server"
    )
    tls_cert_path: Optional[str] = Field(
        default=None,
        description="Path to TLS certificate file"
    )
    tls_key_path: Optional[str] = Field(
        default=None,
        description="Path to TLS private key file"
    )
    tls_ca_path: Optional[str] = Field(
        default=None,
        description="Path to TLS CA certificate file"
    )
    
    @field_validator("log_level")
    def validate_log_level(cls, v):
        """Validate log level is one of the allowed values"""
        allowed = ["debug", "info", "warning", "error", "critical"]
        if v.lower() not in allowed:
            raise ValueError(f"log_level must be one of {allowed}")
        return v.lower()
    
    @field_validator("env")
    def validate_env(cls, v):
        """Validate environment is one of the allowed values"""
        allowed = ["production", "staging", "development", "testnet"]
        if v.lower() not in allowed:
            raise ValueError(f"env must be one of {allowed}")
        return v.lower()
    
    @model_validator(mode='after')
    def validate_cors_settings(self):
        """Warn if CORS wildcard is used in production mode"""
        if self.cors_enabled and "*" in self.cors_origins and self.env == "production":
            import logging
            logger = logging.getLogger(__name__)
            logger.warning(
                "SECURITY WARNING: cors_origins contains '*' (wildcard) in production mode. "
                "This allows any origin to make requests. "
                "Set AILEE_CORS_ORIGINS to a comma-separated list of allowed origins."
            )
        return self

    @model_validator(mode='after')
    def validate_jwt_settings(self):
        """Validate JWT secret is set if JWT is enabled"""
        if self.jwt_enabled and not self.jwt_secret:
            raise ValueError("jwt_secret must be set when jwt_enabled is True")
        if self.jwt_enabled and self.jwt_secret and len(self.jwt_secret) < 32:
            raise ValueError("jwt_secret must be at least 32 characters long for security")
        return self
    
    @model_validator(mode='after')
    def validate_tls_settings(self):
        """Validate TLS configuration"""
        if self.tls_enabled:
            if not self.tls_cert_path or not self.tls_key_path:
                raise ValueError("tls_cert_path and tls_key_path must be set when tls_enabled is True")
        return self
    
    @model_validator(mode='after')
    def validate_bitcoin_rpc_settings(self):
        """Validate Bitcoin RPC configuration"""
        # Warn if using RPC credentials without TLS
        if self.bitcoin_rpc_user and self.bitcoin_rpc_password:
            if not self.bitcoin_rpc_use_tls:
                import logging
                logger = logging.getLogger(__name__)
                logger.warning("Bitcoin RPC credentials provided but TLS is disabled - this is insecure!")
        
        # Parse and validate failover URLs if provided
        if self.bitcoin_rpc_failover_urls:
            urls = [url.strip() for url in self.bitcoin_rpc_failover_urls.split(',')]
            for url in urls:
                if not url.startswith(('http://', 'https://')):
                    raise ValueError(f"Invalid failover URL: {url}")
        
        return self
    
    class Config:
        env_prefix = "AILEE_"
        case_sensitive = False
        env_file = ".env"
        env_file_encoding = "utf-8"


# Global settings instance
settings = Settings()
