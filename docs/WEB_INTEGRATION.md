# Web Integration Guide

## Overview

AILEE Protocol Core now includes a built-in web server that exposes the protocol across the World Wide Web through RESTful APIs and a web-based dashboard.

## Features

- **REST API**: Full REST API for querying node status, metrics, and Layer-2 state
- **Web Dashboard**: Interactive HTML dashboard for monitoring the node
- **CORS Support**: Enabled by default for browser-based access
- **Real-time Updates**: Dashboard auto-refreshes every 10 seconds
- **Cross-platform**: Works on any platform with a modern web browser

## Quick Start

### 1. Build the Web-Enabled Components

```bash
mkdir build
cd build
cmake ..
make
```

This will build:
- `ailee_node` - Main node executable with web server support
- `ailee_web_demo` - Standalone web server demo

### 2. Run the Web Server Demo

```bash
./ailee_web_demo
```

The server will start on `http://localhost:8080`

### 3. Access the Dashboard

Open your web browser and navigate to:
- **Dashboard**: http://localhost:8080/
- **API Documentation**: http://localhost:8080/ (when web/index.html is not found)

## API Endpoints

### Health & Status

#### `GET /api/health`
Health check endpoint - returns basic health status

**Response:**
```json
{
  "status": "healthy",
  "timestamp": 1707874800000
}
```

#### `GET /api/status`
Get detailed node status

**Response:**
```json
{
  "running": true,
  "version": "1.2.1-web-enabled",
  "uptime_seconds": 3600,
  "network": "Bitcoin Mainnet",
  "statistics": {
    "total_transactions": 1000,
    "total_blocks": 100,
    "current_tps": 50.5,
    "pending_tasks": 5
  },
  "last_anchor_hash": "0000000000000000..."
}
```

#### `GET /api/metrics`
Get performance metrics

**Response:**
```json
{
  "timestamp": 1707874800000,
  "node": {
    "type": "AILEE-Core",
    "layer": "Bitcoin Layer-2"
  },
  "performance": {
    "current_tps": 50.5,
    "pending_tasks": 5
  }
}
```

### Layer-2 Operations

#### `GET /api/l2/state`
Get Layer-2 state information

**Response:**
```json
{
  "layer": "Layer-2",
  "protocol": "AILEE-Core",
  "description": "Bitcoin-anchored Layer-2 state",
  "ledger": {
    "status": "active",
    "type": "federated"
  }
}
```

### Orchestration

#### `GET /api/orchestration/tasks`
Get list of orchestration tasks

**Response:**
```json
{
  "tasks": [],
  "total": 0,
  "status": "available"
}
```

#### `POST /api/orchestration/submit`
Submit a new task to the orchestration engine

**Request Body:**
```json
{
  "task_type": "computation",
  "task_data": {
    "description": "Process data",
    "priority": "high"
  }
}
```

**Response:**
```json
{
  "status": "accepted",
  "task_id": "task_1707874800000",
  "message": "Task submitted successfully"
}
```

### Bitcoin Anchoring

#### `GET /api/anchors/latest`
Get the latest Bitcoin anchor information

**Response:**
```json
{
  "message": "Bitcoin anchoring is active",
  "last_anchor_hash": "0000000000000000..."
}
```

## Configuration

### Web Server Configuration

The web server can be configured programmatically:

```cpp
#include "AILEEWebServer.h"

ailee::WebServerConfig config;
config.host = "0.0.0.0";           // Listen on all interfaces
config.port = 8080;                 // Port to listen on
config.enable_cors = true;          // Enable CORS
config.thread_pool_size = 4;        // Thread pool size
config.api_key = "secret-key";      // Optional API key authentication

ailee::AILEEWebServer server(config);
server.start();
```

### API Key Authentication

To enable API key authentication, set the `api_key` in the configuration. All API endpoints (those starting with `/api/`) will require an `X-API-Key` header:

```bash
curl -H "X-API-Key: secret-key" http://localhost:8080/api/status
```

## Security Considerations

### Production Deployment

For production deployments, consider:

1. **Enable API Key Authentication**: Protect your endpoints with API keys
2. **Use HTTPS/TLS**: Enable SSL/TLS for encrypted communication
3. **Firewall Rules**: Restrict access to trusted IP addresses
4. **Rate Limiting**: Implement rate limiting to prevent abuse
5. **Monitoring**: Monitor API usage and detect anomalies

### SSL/TLS Configuration

To enable SSL/TLS (coming soon):

```cpp
config.enable_ssl = true;
config.ssl_cert_path = "/path/to/cert.pem";
config.ssl_key_path = "/path/to/key.pem";
```

## Integration Examples

### JavaScript/Browser

```javascript
// Fetch node status
async function getNodeStatus() {
  const response = await fetch('http://localhost:8080/api/status');
  const data = await response.json();
  console.log('Node Status:', data);
}

// Submit a task
async function submitTask(taskType, taskData) {
  const response = await fetch('http://localhost:8080/api/orchestration/submit', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({
      task_type: taskType,
      task_data: taskData
    })
  });
  return await response.json();
}
```

### Python

```python
import requests

# Get node status
response = requests.get('http://localhost:8080/api/status')
status = response.json()
print(f"Node version: {status['version']}")

# Submit a task
task_data = {
    'task_type': 'computation',
    'task_data': {'description': 'Process data'}
}
response = requests.post('http://localhost:8080/api/orchestration/submit', 
                        json=task_data)
print(response.json())
```

### cURL

```bash
# Health check
curl http://localhost:8080/api/health

# Get status
curl http://localhost:8080/api/status | jq

# Submit task
curl -X POST http://localhost:8080/api/orchestration/submit \
  -H "Content-Type: application/json" \
  -d '{"task_type":"computation","task_data":{"description":"Process data"}}'
```

## Web Dashboard

The web dashboard provides a real-time view of the AILEE node:

- **Node Status**: Current running state, version, uptime, network
- **Performance Metrics**: TPS, pending tasks, total transactions
- **Layer-2 State**: Protocol information, ledger status
- **API Endpoints**: Quick reference to available endpoints

The dashboard auto-refreshes every 10 seconds to show the latest data.

## Troubleshooting

### Port Already in Use

If port 8080 is already in use, change the port in the configuration:

```cpp
config.port = 8081;  // Use a different port
```

### CORS Issues

If you're experiencing CORS issues when accessing from a browser, ensure CORS is enabled:

```cpp
config.enable_cors = true;
```

### Cannot Access Dashboard

Make sure the `web/index.html` file is in the current working directory when you run the server. The server looks for `web/index.html` relative to where it's executed.

## Future Enhancements

Planned features for future releases:

- WebSocket support for real-time streaming updates
- GraphQL API endpoint
- Enhanced authentication (OAuth2, JWT)
- Rate limiting and request throttling
- Admin dashboard with write capabilities
- Prometheus metrics endpoint
- OpenAPI/Swagger documentation endpoint

## Contributing

To contribute to the web integration:

1. Review the code in `src/AILEEWebServer.cpp` and `include/AILEEWebServer.h`
2. Test new endpoints thoroughly
3. Update this documentation
4. Submit a pull request

## License

MIT License - Same as AILEE Protocol Core

## Support

For issues or questions about the web integration:
- GitHub Issues: https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/issues
- Documentation: https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/tree/main/docs
