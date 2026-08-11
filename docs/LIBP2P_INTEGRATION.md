# libp2p C++ Bindings Integration Guide

## Overview

AILEE-Core includes comprehensive support for libp2p C++ bindings to enable true peer-to-peer networking capabilities. The implementation supports both:

1. **Production Mode**: Full libp2p integration when library is available
2. **Development Mode**: Enhanced stub implementation for development and testing

## Architecture

### P2P Network Layer

The P2P networking layer is implemented in:
- **Header**: `src/network/P2PNetwork.h`
- **Implementation**: `src/network/P2PNetwork.cpp`
- **Integration**: `src/orchestration/DistributedTaskProtocol.cpp`

### Key Features

✅ **Peer Discovery**
- mDNS (multicast DNS) for local network discovery
- Kademlia DHT for global peer routing
- Bootstrap peer support for initial network entry

✅ **Messaging**
- GossipSub pub/sub protocol for topic-based broadcasting
- Direct peer-to-peer streams for request/response
- Topic subscriptions with callback handlers

✅ **Connection Management**
- Multiaddress format support (`/ip4/x.x.x.x/tcp/4001`)
- Peer lifecycle management (connect/disconnect)
- Network statistics and monitoring

✅ **Security**
- Peer identity via public/private key pairs
- Persistent peer ID storage
- Secure stream multiplexing

## Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake git libssl-dev

# macOS
brew install cmake openssl git
```

### Option 1: Install cpp-libp2p from Source

```bash
# Clone cpp-libp2p repository
git clone --recursive https://github.com/libp2p/cpp-libp2p.git
cd cpp-libp2p

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install

# Verify installation
ldconfig -p | grep libp2p
```

### Option 2: Install via Hunter (Recommended for Production)

The cpp-libp2p library uses Hunter package manager. To integrate:

1. Add Hunter support to your project:

```cmake
# In CMakeLists.txt, before project()
include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.25.5.tar.gz"
    SHA1 "a20151e4c0740ee7d0f9994476856d813cdead29"
)
```

2. Add libp2p to your dependencies:

```cmake
hunter_add_package(libp2p)
find_package(libp2p REQUIRED)
```

### Option 3: Build AILEE-Core Without libp2p

The system gracefully degrades to enhanced stub mode:

```bash
cd /path/to/AILEE-Protocol-Core-For-Bitcoin
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Building AILEE-Core with libp2p

### Standard Build

```bash
cd /path/to/AILEE-Protocol-Core-For-Bitcoin
mkdir build && cd build

# Configure with libp2p support
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DAILEE_HAS_LIBP2P=1

# Build
make -j$(nproc)

# Verify
./ailee_node --help
```

### Build Outputs

When libp2p is found:
```
-- libp2p found: /usr/local/lib/libp2p.so
-- libp2p support enabled
```

When libp2p is not found:
```
⚠️  WARNING: libp2p not found
    P2P networking will use stub implementation
    To enable libp2p: Install cpp-libp2p and rebuild
```

## Configuration

### P2P Network Configuration

Create or modify `config/p2p_network.yaml`:

```yaml
p2p:
  # Network listening address
  listenAddress: "/ip4/0.0.0.0/tcp/4001"
  
  # Bootstrap peers for initial network discovery
  bootstrapPeers:
    - "/ip4/bootstrap1.ailee.network/tcp/4001/p2p/QmBootstrap1PeerID"
    - "/ip4/bootstrap2.ailee.network/tcp/4001/p2p/QmBootstrap2PeerID"
  
  # Path to store/load private key for peer identity
  privateKeyPath: "./data/p2p_private_key"
  
  # Maximum number of peers to maintain
  maxPeers: 50
  
  # Enable mDNS for local peer discovery
  enableMDNS: true
  
  # Enable Kademlia DHT for peer routing
  enableDHT: true
```

### Programmatic Configuration

```cpp
#include "network/P2PNetwork.h"

using namespace ailee::network;

P2PConfig config;
config.listenAddress = "/ip4/0.0.0.0/tcp/4001";
config.bootstrapPeers = {
    "/ip4/192.168.1.100/tcp/4001",
    "/dnsaddr/bootstrap.ailee.network/tcp/4001"
};
config.privateKeyPath = "./data/p2p_key";
config.maxPeers = 100;
config.enableMDNS = true;
config.enableDHT = true;

P2PNetwork network(config);
network.start();
```

## Usage Examples

### Basic Pub/Sub Messaging

```cpp
#include "network/P2PNetwork.h"
#include <iostream>

using namespace ailee::network;

int main() {
    // Create and start network
    P2PNetwork network;
    network.start();
    
    // Subscribe to a topic
    network.subscribe("ailee.tasks", [](const NetworkMessage& msg) {
        std::cout << "Received task from: " << msg.senderId << std::endl;
        std::cout << "Payload size: " << msg.payload.size() << " bytes" << std::endl;
        
        // Process the message
        // ...
    });
    
    // Publish a message
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    network.publish("ailee.tasks", payload);
    
    // Keep running
    std::this_thread::sleep_for(std::chrono::hours(1));
    
    network.stop();
    return 0;
}
```

### Direct Peer Communication

```cpp
#include "network/P2PNetwork.h"

using namespace ailee::network;

// Send request to specific peer
std::string targetPeer = "QmPeerID123...";
std::string protocol = "/ailee/task/1.0.0";
std::vector<uint8_t> request = serializeTaskRequest(task);

auto response = network.sendToPeer(targetPeer, protocol, request);
if (response) {
    auto result = deserializeTaskResult(*response);
    processResult(result);
} else {
    std::cerr << "Failed to get response from peer" << std::endl;
}
```

### Peer Discovery and Management

```cpp
#include "network/P2PNetwork.h"

// Get list of connected peers
auto peers = network.getPeers();
for (const auto& peer : peers) {
    std::cout << "Peer: " << peer.peerId << std::endl;
    std::cout << "  Address: " << peer.multiaddr << std::endl;
    std::cout << "  Latency: " << peer.latencyMs << "ms" << std::endl;
    std::cout << "  Connected: " << (peer.connected ? "yes" : "no") << std::endl;
}

// Connect to a specific peer
network.connectToPeer("/ip4/192.168.1.200/tcp/4001");

// Disconnect from a peer
network.disconnectPeer("QmPeerIDToDisconnect");
```

### Network Statistics

```cpp
auto stats = network.getStats();
std::cout << "Connected peers: " << stats.connectedPeers << std::endl;
std::cout << "Messages sent: " << stats.totalMessagesSent << std::endl;
std::cout << "Messages received: " << stats.totalMessagesReceived << std::endl;
std::cout << "Bytes uploaded: " << stats.bytesUploaded << std::endl;
std::cout << "Bytes downloaded: " << stats.bytesDownloaded << std::endl;
```

## Integration with Distributed Task Protocol

The P2P network integrates seamlessly with AILEE's task distribution system:

```cpp
#include "network/P2PNetwork.h"
#include "orchestration/DistributedTaskProtocol.h"

using namespace ailee::network;
using namespace ailee::orchestration;

// Create P2P network
auto p2pNetwork = std::make_shared<P2PNetwork>();
p2pNetwork->start();

// Create distributed task protocol using the P2P network
DistributedTaskProtocol::Config taskConfig;
taskConfig.nodeId = "node-1";
taskConfig.maxConcurrentTasks = 10;

DistributedTaskProtocol taskProtocol(taskConfig, p2pNetwork);
taskProtocol.start();

// Submit tasks - they will be automatically distributed via P2P
Task task;
task.id = "task-123";
task.type = "compute";
task.priority = TaskPriority::High;

taskProtocol.submitTask(task);
```

## Troubleshooting

### libp2p Not Found

**Problem**: CMake cannot find libp2p during configuration

**Solution**:
```bash
# Ensure libp2p is installed in standard location
sudo ldconfig

# Or specify custom path
cmake .. -DLIBP2P_ROOT=/path/to/libp2p

# Or use pkg-config
export PKG_CONFIG_PATH=/path/to/libp2p/lib/pkgconfig:$PKG_CONFIG_PATH
```

### Compilation Errors

**Problem**: Undefined references to libp2p symbols

**Solution**:
1. Verify libp2p installation: `ldconfig -p | grep libp2p`
2. Check CMake found correct library: Look for "libp2p found" in cmake output
3. Rebuild from clean state: `rm -rf build && mkdir build && cd build && cmake ..`

### Runtime Connection Issues

**Problem**: Peers cannot discover each other

**Solution**:
1. Check firewall settings allow TCP port 4001
2. Verify bootstrap peers are reachable
3. Enable mDNS for local network discovery
4. Check network logs: `AILEE_LOG_LEVEL=debug ./ailee_node`

## Performance Tuning

### Optimize for Low Latency

```cpp
P2PConfig config;
config.maxPeers = 20;  // Limit peers for lower overhead
config.enableMDNS = true;  // Fast local discovery
config.enableDHT = false;  // Disable DHT for reduced overhead
```

### Optimize for High Throughput

```cpp
P2PConfig config;
config.maxPeers = 100;  // More peers for redundancy
config.enableMDNS = true;
config.enableDHT = true;  // Better routing
```

### Optimize for Large Networks

```cpp
P2PConfig config;
config.maxPeers = 200;
config.enableMDNS = false;  // Not useful at scale
config.enableDHT = true;  // Essential for discovery
config.bootstrapPeers = {/* multiple bootstrap nodes */};
```

## Security Considerations

### Peer Identity

- Each node has a unique peer ID derived from its public key
- Private keys are stored in `privateKeyPath` (default: `./data/p2p_private_key`)
- **Important**: Keep private keys secure and backed up

### Network Security

- All connections use secure transport (SECIO or Noise)
- Message signing ensures authenticity
- Consider using allowlists for production deployments

### Best Practices

1. **Rotate Keys**: Periodically generate new peer identities
2. **Monitor Traffic**: Track network statistics for anomalies
3. **Rate Limiting**: Implement application-level rate limits
4. **Validation**: Always validate message contents before processing

## References

- [libp2p Specification](https://github.com/libp2p/specs)
- [cpp-libp2p GitHub](https://github.com/libp2p/cpp-libp2p)
- [GossipSub Specification](https://github.com/libp2p/specs/tree/master/pubsub/gossipsub)
- [Kademlia DHT](https://github.com/libp2p/specs/tree/master/kad-dht)
- [Multiaddress Format](https://github.com/multiformats/multiaddr)

## Support

For issues or questions:
- GitHub Issues: https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/issues
- Documentation: See `docs/` directory
- Examples: See `examples/` directory

## License

This implementation is released under the MIT License. See LICENSE file for details.
