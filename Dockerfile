# ============================
# Stage 1 — Build C++ Node
# ============================
FROM ubuntu:22.04 AS cpp-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake git wget autoconf automake libtool pkg-config \
    libssl-dev libcurl4-openssl-dev libzmq3-dev \
    libjsoncpp-dev libyaml-cpp-dev librocksdb-dev \
    && rm -rf /var/lib/apt/lists/*

# Install python3 for metadata generation
RUN apt-get update && apt-get install -y python3

# --------------------------------------------------
# Build libsecp256k1 from source (Bitcoin Core style)
# --------------------------------------------------
WORKDIR /tmp/secp256k1
RUN git clone https://github.com/bitcoin-core/secp256k1.git . && \
    ./autogen.sh && \
    ./configure --enable-module-recovery --enable-experimental && \
    make -j2 && \
    make install

WORKDIR /build

# Copy full repo
COPY . .

# Build the C++ node
RUN rm -rf build && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DAILEE_USE_RUST_PROVER=OFF && \
    make -j2 ailee_node


# ============================
# Stage 2 — Runtime Image
# ============================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1

# Install Python 3.11 + runtime libs + build deps (REQUIRED for psutil)
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3.11 python3-pip python3.11-venv python3.11-dev \
    gcc build-essential \
    curl libssl3 libcurl4 libzmq5 libjsoncpp25 \
    libyaml-cpp0.7 librocksdb6.11 libstdc++6 procps \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set python3.11 as default
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.11 1 && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3.11 1

WORKDIR /app

# Install Python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Create runtime user + directories
RUN useradd -m -u 1000 ailee && \
    mkdir -p /app/logs /data && \
    chown -R ailee:ailee /app/logs /data

# Copy application code
COPY --chown=ailee:ailee api/ ./api/
COPY --chown=ailee:ailee web/ ./web/

# Optional post-deploy script
COPY --chown=ailee:ailee scripts/railway_post_deploy.sh /app/post_deploy.sh
RUN chmod +x /app/post_deploy.sh

# Copy C++ node + config from builder
COPY --from=cpp-builder --chown=ailee:ailee /build/build/ailee_node ./ailee_node
COPY --from=cpp-builder --chown=ailee:ailee /build/config ./config
RUN chmod +x ./ailee_node


# ============================
# Start Script (Render-Compatible)
# ============================
RUN cat << 'EOF' > /app/start.sh
#!/bin/bash
set -e

mkdir -p /data

echo "--- System Diagnostics ---"
ulimit -a
free -m
df -h
echo "--------------------------"

# Render requires listening on $PORT
export AILEE_WEB_SERVER_PORT=${PORT}
echo "Starting C++ node on :${AILEE_WEB_SERVER_PORT}..."
./ailee_node --web --heartbeat > /app/logs/cpp-node.log 2>&1 &
CPP_PID=$!
sleep 3

if ! kill -0 $CPP_PID 2>/dev/null; then
    echo "WARNING: C++ node failed to start"
    cat /app/logs/cpp-node.log
else
    echo "C++ node started on :${AILEE_WEB_SERVER_PORT}"
fi

# Python API also listens on $PORT
API_PORT=${PORT}
echo "Starting Python API on :${API_PORT}..."
export AILEE_PORT=${API_PORT}
export AILEE_NODE_URL="http://localhost:${AILEE_WEB_SERVER_PORT}"

exec uvicorn api.main:app --host 0.0.0.0 --port ${API_PORT} --log-level info
EOF

RUN chmod +x /app/start.sh

EXPOSE 8080

CMD ["/app/start.sh"]
