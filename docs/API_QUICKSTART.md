# AILEE-Core REST API - Quick Start Guide

## 🚀 Local Development

### Run Locally (Python)
```bash
# Install dependencies
pip install -r requirements.txt

# Run the API
uvicorn api.main:app --host 0.0.0.0 --port 8080 --reload

# Access the API
curl http://localhost:8080/health

# View documentation
open http://localhost:8080/docs
```

### Run with Docker
```bash
# Build the image
docker build -t ailee-core-api .

# Run the container
docker run -p 8000:8000 ailee-core-api

# Test the API
curl http://localhost:8000/health

# Access the web dashboard
open http://localhost:8000/
```

## ☁️ Deploy to Fly.io

### First-time Setup
```bash
# Install Fly CLI
curl -L https://fly.io/install.sh | sh

# Login to Fly.io
fly auth login

# Launch the app (first time)
fly launch

# Set secrets
fly secrets set AILEE_NODE_ID=my-node-1
fly secrets set AILEE_JWT_SECRET=$(openssl rand -base64 32)
```

### Deploy Updates
```bash
# Deploy changes
fly deploy

# Check status
fly status

# View logs
fly logs

# Open in browser
fly open
```

### Scale
```bash
# Scale instances
fly scale count 2

# Scale memory
fly scale memory 512
```

## 🔧 Configuration

Copy `.env.example` to `.env` and customize:

```bash
AILEE_NODE_ID=my-node-1
AILEE_ENV=production
AILEE_LOG_LEVEL=info
AILEE_JWT_ENABLED=false
```

## 📊 API Endpoints

### Core Endpoints
- `GET /` - API information
- `GET /health` - Health check
- `GET /status` - Node status
- `GET /metrics` - Node metrics
- `GET /docs` - OpenAPI documentation

### Trust & Validation
- `POST /trust/score` - Compute trust score
- `POST /trust/validate` - Validate output

### Layer-2 State
- `GET /l2/state` - L2 state snapshot
- `GET /l2/anchors` - Anchor history

### Transactions ⭐ NEW
- `POST /transactions/submit` - Submit transaction
- `GET /transactions/list` - List all transactions
- `GET /transactions/hash/{tx_hash}` - Get transaction by hash
- `GET /transactions/address/{address}` - Get transactions by address

For detailed transaction API documentation, see [docs/TRANSACTION_API.md](docs/TRANSACTION_API.md)

## 🧪 Testing

### Basic Health Check
```bash
# Health check
curl http://localhost:8080/health

# Node status
curl http://localhost:8080/status

# Trust score
curl -X POST http://localhost:8080/trust/score \
  -H "Content-Type: application/json" \
  -d '{"input_data": "test"}'

# L2 state
curl http://localhost:8080/l2/state

# Metrics
curl http://localhost:8080/metrics
```

### Transaction Submission ⭐ NEW
```bash
# Submit a transaction
curl -X POST http://localhost:8080/transactions/submit \
  -H "Content-Type: application/json" \
  -d '{
    "from_address": "alice",
    "to_address": "bob",
    "amount": 1000,
    "data": "Payment for services"
  }'

# List all transactions
curl http://localhost:8080/transactions/list

# Get transaction by hash
curl http://localhost:8080/transactions/hash/{tx_hash}

# Get transactions for an address
curl http://localhost:8080/transactions/address/alice
```

### Testing C++ Node Connectivity

#### From Host Machine
```bash
# Test Python API
curl http://localhost:8080/health

# Test C++ node (if running locally)
curl http://localhost:8080/api/health
```

#### From Inside Docker Container
```bash
# Access running container
docker exec -it <container-name> bash

# Test connectivity to C++ node
# For localhost setup:
curl http://localhost:8080/api/health

# For Docker Compose setup (C++ node in separate container):
curl http://ailee-node-1:8080/api/health

# For Fly.io deployment (C++ node as separate app):
curl https://<cpp-node-app>.fly.dev/api/health

# Check DNS resolution (Docker Compose)
nslookup ailee-node-1

# Check if port is reachable
nc -zv localhost 8080
nc -zv ailee-node-1 8080
```

#### Debugging Connection Issues
```bash
# Check environment variable
echo $AILEE_NODE_URL

# Test with explicit URL
curl http://localhost:8080/api/health -v

# Check if service is listening on 0.0.0.0 (not 127.0.0.1)
netstat -tlnp | grep 8080

# View Python API logs for C++ connection status
docker logs <container-name>
```

### Deployment Architecture

The AILEE Protocol supports multiple deployment configurations:

#### 1. **Local Development** (Default)
- Python API: `localhost:8080`
- C++ Node: `localhost:8080` (if running)
- Set `AILEE_NODE_URL=http://localhost:8080`

#### 2. **Docker Compose** (Multi-container)
- Python API: Container on port 8080
- C++ Node: Separate container (`ailee-node-1`)
- Set `AILEE_NODE_URL=http://ailee-node-1:8080`

#### 3. **Fly.io - Separate Apps**
- Python API: One Fly app
- C++ Node: Separate Fly app
- Set `AILEE_NODE_URL=https://<cpp-node-app>.fly.dev`

#### 4. **Fly.io - Sidecar** (Not recommended)
- Python API and C++ Node in same container
- Set `AILEE_NODE_URL=http://localhost:8080`
- Note: Requires custom Dockerfile combining both services

### Configuration Examples

#### Docker Compose Example
```yaml
# docker-compose.yml
services:
  python-api:
    build:
      dockerfile: Dockerfile
    environment:
      - AILEE_NODE_URL=http://ailee-node-1:8080
    ports:
      - "8080:8080"
    depends_on:
      - ailee-node-1
  
  ailee-node-1:
    build:
      dockerfile: Dockerfile.node
    ports:
      - "8080:8080"
```

#### Fly.io Example (Separate Apps)
```bash
# Deploy C++ node first
fly launch --dockerfile Dockerfile.node --name ailee-cpp-node

# Deploy Python API with C++ node URL
fly secrets set AILEE_NODE_URL=https://ailee-cpp-node.fly.dev
fly deploy
```

## 📚 Documentation

- OpenAPI/Swagger: `http://localhost:8080/docs`
- ReDoc: `http://localhost:8080/redoc`
- OpenAPI JSON: `http://localhost:8080/openapi.json`

## 🔒 Security

- JWT auth disabled by default for easy testing
- Enable in production: `AILEE_JWT_ENABLED=true`
- Rate limiting: 100 requests per 60 seconds
- Non-root Docker user
- Health check endpoint for monitoring

## 💡 Tips

1. Use environment variables for configuration
2. Enable JWT auth in production
3. Monitor health check endpoint
4. Review logs regularly
5. Start with minimal resources (256MB RAM)
6. Scale as needed based on traffic
