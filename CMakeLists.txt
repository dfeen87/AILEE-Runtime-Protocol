cmake_minimum_required(VERSION 3.16)
project(AILEE_Core VERSION 5.3.0 LANGUAGES CXX)

# ============================================================================
# Compiler & Standard Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Build Metadata Generation (Deterministic)
# ============================================================================
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE AILEE_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT AILEE_COMMIT_HASH)
    set(AILEE_COMMIT_HASH "unknown")
endif()

# Fallbacks for deterministic build variables that can be overridden via CMake
if(NOT AILEE_BUILD_NUMBER)
    set(AILEE_BUILD_NUMBER "0")
endif()
if(NOT AILEE_PROTOCOL_VERSION)
    set(AILEE_PROTOCOL_VERSION "1.0.0")
endif()

# Build type default to Release for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimization flags (Critical for TPS Engine)
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native") # Optimize for local CPU
endif()

# ============================================================================
# Dependencies
# ============================================================================

# 1. OpenSSL (Required for SHA256 in Recovery Protocol & Gold Bridge)
find_package(OpenSSL REQUIRED)

# 2. CURL (Required for BitcoinRPCClient.h & BitcoinAdapter RPC)
find_package(CURL REQUIRED)

# 3. ZeroMQ & cppzmq (Required for BitcoinZMQListener.h)
# We use PkgConfig as a fallback for Linux/Unix environments
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBZMQ QUIET libzmq)
endif()

find_package(ZeroMQ QUIET)
if(NOT ZeroMQ_FOUND)
    # If standard find fails, try PkgConfig hints
    find_path(ZeroMQ_INCLUDE_DIR zmq.h HINTS ${PC_LIBZMQ_INCLUDE_DIRS})
    find_library(ZeroMQ_LIBRARY zmq HINTS ${PC_LIBZMQ_LIBRARY_DIRS})
endif()

# Find the C++ bindings (header only usually)
find_package(cppzmq QUIET)

# 4. JsonCpp (For RPC parsing in BitcoinAdapter)
find_package(jsoncpp CONFIG QUIET)

# 5. nlohmann/json (Header-only JSON for multi-chain adapters)
find_package(nlohmann_json QUIET)

# 6. yaml-cpp (Required for policy config hot-reload system)
find_package(yaml-cpp QUIET)

# 7. Threads (Required for running ZMQ listener in background)
find_package(Threads REQUIRED)

# Try pkg-config for libsecp256k1 first
if(PKG_CONFIG_FOUND)
    pkg_check_modules(libsecp256k1 QUIET libsecp256k1)
endif()
if(NOT libsecp256k1_FOUND)
    # Fallback to normal find_library
    find_library(libsecp256k1_LIBRARY NAMES secp256k1)
    find_path(libsecp256k1_INCLUDE_DIR NAMES secp256k1.h)
    if(libsecp256k1_LIBRARY AND libsecp256k1_INCLUDE_DIR)
        set(libsecp256k1_FOUND TRUE)
        set(libsecp256k1_LIBRARIES ${libsecp256k1_LIBRARY})
        set(libsecp256k1_INCLUDE_DIRS ${libsecp256k1_INCLUDE_DIR})
    endif()
endif()
if(libsecp256k1_FOUND)
    message(STATUS "Found libsecp256k1: ${libsecp256k1_LIBRARIES}")
    include_directories(${libsecp256k1_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "libsecp256k1 is required but not found")
endif()

# 8. RocksDB (Optional - for persistent storage)
# Robust detection logic prioritizing pkg-config for Linux CI environments
set(ROCKSDB_LINK_LIBS "")
set(RocksDB_FOUND FALSE)

# Try pkg-config first (most reliable on Ubuntu/Debian)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ROCKSDB QUIET rocksdb)
    if(PC_ROCKSDB_FOUND)
        set(RocksDB_FOUND TRUE)
        set(ROCKSDB_LINK_LIBS ${PC_ROCKSDB_LIBRARIES})
        include_directories(${PC_ROCKSDB_INCLUDE_DIRS})
        message(STATUS "RocksDB found via pkg-config: ${ROCKSDB_LINK_LIBS}")
    endif()
endif()

# Fallback to CMake find_package
if(NOT RocksDB_FOUND)
    find_package(RocksDB QUIET)
    if(RocksDB_FOUND)
        message(STATUS "RocksDB found via find_package")
        if(TARGET RocksDB::rocksdb)
            set(ROCKSDB_LINK_LIBS RocksDB::rocksdb)
        elseif(TARGET rocksdb::rocksdb)
            set(ROCKSDB_LINK_LIBS rocksdb::rocksdb)
        elseif(TARGET rocksdb)
            set(ROCKSDB_LINK_LIBS rocksdb)
        elseif(DEFINED ROCKSDB_LIBRARIES)
            set(ROCKSDB_LINK_LIBS ${ROCKSDB_LIBRARIES})
        elseif(DEFINED RocksDB_LIBRARIES)
            set(ROCKSDB_LINK_LIBS ${RocksDB_LIBRARIES})
        else()
            set(ROCKSDB_LINK_LIBS rocksdb)
        endif()

        if(DEFINED ROCKSDB_INCLUDE_DIR)
            include_directories(${ROCKSDB_INCLUDE_DIR})
        elseif(DEFINED RocksDB_INCLUDE_DIR)
            include_directories(${RocksDB_INCLUDE_DIR})
        endif()
    endif()
endif()

# Manual fallback: locate rocksdb library and headers directly via CMake search
if(NOT RocksDB_FOUND)
    find_library(ROCKSDB_LIBRARY_PATH NAMES rocksdb)
    find_path(ROCKSDB_HEADER_PATH rocksdb/db.h)
    if(ROCKSDB_LIBRARY_PATH AND ROCKSDB_HEADER_PATH)
        set(RocksDB_FOUND TRUE)
        set(ROCKSDB_LINK_LIBS ${ROCKSDB_LIBRARY_PATH})
        include_directories(${ROCKSDB_HEADER_PATH})
        message(STATUS "RocksDB found via manual search: ${ROCKSDB_LIBRARY_PATH}")
    endif()
endif()

# 9. libp2p (Optional - for P2P networking)
# Add cmake module path for custom FindLibp2p
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Libp2p QUIET)
if(LIBP2P_FOUND)
    message(STATUS "libp2p found: ${LIBP2P_LIBRARIES}")
else()
    message(WARNING "libp2p not found - P2P networking will use stub implementation")
    message(STATUS "To enable libp2p: Install cpp-libp2p from https://github.com/libp2p/cpp-libp2p")
endif()

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include # Best practice to keep headers here
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/l1
    ${CMAKE_CURRENT_SOURCE_DIR}/src/l2
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/security
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recovery
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/metrics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/orchestration
    ${CMAKE_CURRENT_SOURCE_DIR}/econ  # For ILedger.h and economic model headers
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)

if(ZeroMQ_INCLUDE_DIR)
    include_directories(${ZeroMQ_INCLUDE_DIR})
endif()

# Add yaml-cpp include if found
if(yaml-cpp_FOUND)
    include_directories(${YAML_CPP_INCLUDE_DIR})
endif()

# Add RocksDB include if found
if(RocksDB_FOUND)
    include_directories(${RocksDB_INCLUDE_DIRS})
endif()

# Add libp2p include if found
if(LIBP2P_FOUND)
    include_directories(${LIBP2P_INCLUDE_DIRS})
endif()

# ============================================================================
# Source & Header Files
# ============================================================================

# Explicitly list headers so they appear in IDEs (VS Code / CLion)
set(AILEE_HEADERS
    src/l2/ailee_tps_engine.h
    src/l2/ailee_gold_bridge.h
    src/recovery/ailee_recovery_protocol.h
    src/l1/BitcoinZMQListener.h
    src/l1/BitcoinRPCClient.h
    src/telemetry/ailee_energy_telemetry.h
    src/security/ailee_circuit_breaker.h
    src/l2/ailee_sidechain_bridge.h
    include/ailee_dao_governance.h
    include/Global_Seven.h
    include/AmbientAI.h
    include/L2State.h
    include/BlockProducer.h
    include/AILEEWebServer.h
    include/BuildInfo.h
    include/ConfigInfo.h
    include/L1Reflection.h
    include/HeartbeatLog.h
    include/StateLog.h
    include/MeshCoherence.h
)

# Policy System Headers (new files)
set(POLICY_HEADERS
    include/policies.h
    include/metrics.h
    include/expr.h
    config/config_types.h
    config/config_loader.h
    config/config_hot_reload.h
    include/config_types.h
    include/config_loader.h
    include/config_hot_reload.h
)

# Adapter implementation sources
file(GLOB AMBIENT_NODE_SOURCES "src/ambient_node/*.cpp")
file(GLOB AMBIENT_MESH_SOURCES "src/ambient_mesh/*.cpp")
file(GLOB PARTICIPATION_SOURCES "src/participation/*.cpp")
file(GLOB CONSENSUS_SOURCES "src/consensus/*.cpp")
file(GLOB RUNTIME_SOURCES "src/runtime/ambient_*.cpp" "src/runtime/BitVM*.cpp" "src/runtime/Wasm*.cpp")
file(GLOB PROTOCOL_SOURCES "src/protocol/*.cpp")
file(GLOB ENERGY_SOURCES "src/energy/*.cpp")
file(GLOB AMBIENT_SOURCES_2 "src/ambient/*.cpp")
file(GLOB ANCHORING_SOURCES "src/anchoring/*.cpp")
file(GLOB IDENTITY_SOURCES "src/identity/*.cpp")

set(ADAPTER_SOURCES
    src/l1/BitcoinZMQListener.cpp
    src/l1/BitcoinAdapter.cpp
    src/l1/EthereumAdapter.cpp
    src/l1/PolygonAdapter.cpp
    src/l1/SolanaAdapter.cpp
    src/l1/AvalancheAdapter.cpp
    src/l1/LitecoinAdapter.cpp
    src/l1/DogecoinAdapter.cpp
    src/l1/AdapterRegistry.cpp
    src/l1/AILEEMempoolAdapter.cpp
    src/l1/AILEENetworkAdapter.cpp
    src/l1/AILEEEnergyAdapter.cpp
    src/l1/EvmBaseAdapter.cpp
    src/l1/CardanoAdapter.cpp
    src/l1/PolkadotAdapter.cpp
    src/l1/LitecoinAdapter.cpp
    src/l1/DogecoinAdapter.cpp
    src/l1/AdapterRegistry.cpp
)

# AmbientAI Sources (new mesh intelligence layer)
set(AMBIENT_SOURCES
    src/AmbientAI.cpp
)

# WNN Sources (Wave Native Network V11)
set(WNN_SOURCES
    src/wnn/core/wave_native_network_core.cpp
    src/wnn/propagation/wave_phase_propagation.cpp
    src/wnn/resonance/wave_resonance_profiles.cpp
    src/wnn/identity/wave_identity_phase_binding.cpp
    src/wnn/state_machine/wave_epoch_phase_state_machine.cpp
    src/wnn/routing/wave_native_routing_layer.cpp
    src/wnn/mesh/wave_mesh_integration.cpp
)

# AILEE CORE Integration Sources
set(AILEE_CORE_SOURCES
    src/ReflectionLayer.cpp
    src/NodeIdentity.cpp
    src/MeshCoherence.cpp
    src/ailee_core/runtime/ailee_core_runtime.cpp
    src/ailee_core/identity/ailee_core_identity.cpp
    src/ailee_core/participation/ailee_core_participation.cpp
    src/ailee_core/consensus/ailee_core_consensus.cpp
    src/ailee_core/mesh/ailee_core_mesh.cpp
    src/ailee_core/anchoring/ailee_core_anchoring.cpp
    src/l2/DeterministicEngine.cpp
    src/l2/EpochScheduler.cpp
    src/l2/StateRootPipeline.cpp
    src/l2/RecoveryPayload.cpp
    src/l2/ExecutionContext.cpp
    src/heartbeat/WavePhaseController.cpp
    src/heartbeat/EpochScheduler.cpp
    src/heartbeat/DeterministicEpochExecutorWrapper.cpp
    src/heartbeat/BitcoinAnchoringInterface.cpp
    src/heartbeat/HeartbeatOrchestrator.cpp
)


# New Infrastructure Sources
set(INFRASTRUCTURE_SOURCES
    src/storage/PersistentStorage.cpp
    src/network/P2PNetwork.cpp
    src/network/LogicalClock.cpp
    src/network/ReputationRateLimiter.cpp
    src/orchestration/DistributedTaskProtocol.cpp
    src/metrics/PrometheusExporter.cpp
    src/build/BuildInfo.cpp
)

# Policy System Sources
set(POLICY_SOURCES
    src/security/policies.cpp
    src/telemetry/metrics.cpp
    src/policies.cpp
    src/metrics.cpp
    src/expr.cpp
    src/config_loader.cpp
    src/config_hot_reload.cpp
)

set(AILEE_L2_SOURCES
    src/core_spine/ailee_api.cpp
    src/posture_engine/posture_engine.cpp
    src/policy_layer/policy_layer.cpp
    src/policy_layer/config_parser.cpp
    src/runtime/BitVMInterpreter.cpp
)

# Main Executable Sources
set(CORE_SOURCES
    src/main.cpp
    src/orchestration/Orchestrator.cpp
    src/orchestration/ProverSwarm.cpp
    src/security/ailee_circuit_breaker.cpp
    src/security/zk_proofs.cpp
    src/l3/NetworkReflection.cpp
    src/l3/NetworkBinding.cpp
    src/l3/GossipLayer.cpp
    src/l3/PeerSync.cpp
    src/l3/NetworkDriver.cpp
    src/l1/AnchorCommitment.cpp
    src/l1/ReorgDetector.cpp
    src/l1/SettlementIngestion.cpp
    src/l2/L2State.cpp
    src/l2/BlockProducer.cpp
    src/l2/Mempool.cpp
    src/AILEEWebServer.cpp
    ${ADAPTER_SOURCES}
    ${AMBIENT_NODE_SOURCES}
    ${AMBIENT_MESH_SOURCES}
    ${PARTICIPATION_SOURCES}
    ${CONSENSUS_SOURCES}
    ${RUNTIME_SOURCES}
    ${PROTOCOL_SOURCES}
    ${ENERGY_SOURCES}
    ${AMBIENT_SOURCES_2}
    ${ANCHORING_SOURCES}
    ${IDENTITY_SOURCES}
    ${AMBIENT_SOURCES}
    ${WNN_SOURCES}
    ${AILEE_CORE_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${POLICY_SOURCES}
    ${AILEE_HEADERS}
    ${POLICY_HEADERS}
)

# ============================================================================
# Build Targets
# ============================================================================

# --- 1. The Main Node Executable ---
# ============================================================================
# Rust Prover Integration
# ============================================================================

option(AILEE_USE_RUST_PROVER "Build and link the real Rust Halo2 prover instead of C++ fallback" ON)

if(AILEE_USE_RUST_PROVER)
    message(STATUS "Integrating Rust Halo2 Prover (disabling C++ fallback)")

    set(RUST_PROVER_LIB_PATH "${CMAKE_BINARY_DIR}/libailee_rust_prover.a")

    add_custom_target(cargo_build_rust_prover ALL
        COMMAND cargo build --release
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/rust_prover
    )

    add_custom_command(
        TARGET cargo_build_rust_prover POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/src/rust_prover/target/release/libailee_rust_prover.a
                ${RUST_PROVER_LIB_PATH}
    )

    add_library(ailee_rust_prover STATIC IMPORTED GLOBAL)
    add_dependencies(ailee_rust_prover cargo_build_rust_prover)
    set_target_properties(ailee_rust_prover PROPERTIES IMPORTED_LOCATION ${RUST_PROVER_LIB_PATH})


else()
    message(STATUS "Using C++ Mock Fallback for Rust Prover")
endif()

include_directories(${CMAKE_BINARY_DIR})

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/build_metadata.hpp
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/generate_build_metadata.py
            --out-json /dev/null
            --out-hpp ${CMAKE_BINARY_DIR}/build_metadata.hpp
            --rust-prover-lib ${CMAKE_BINARY_DIR}/libailee_rust_prover.a

)

add_custom_target(generate_metadata_hpp DEPENDS ${CMAKE_BINARY_DIR}/build_metadata.hpp)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/metadata.json
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/generate_build_metadata.py
            --out-json ${CMAKE_BINARY_DIR}/metadata.json
            --out-hpp /dev/null
            --node-bin ${CMAKE_BINARY_DIR}/ailee_node
            --service-bin ${CMAKE_BINARY_DIR}/ailee_service
            --rust-prover-lib ${CMAKE_BINARY_DIR}/libailee_rust_prover.a
    DEPENDS ailee_node ailee_service
)

add_custom_target(generate_metadata ALL DEPENDS ${CMAKE_BINARY_DIR}/metadata.json)

add_custom_target(verify_build ALL
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/verify_build_hashes.py
            --metadata ${CMAKE_BINARY_DIR}/metadata.json
            --node ${CMAKE_BINARY_DIR}/ailee_node
            --service ${CMAKE_BINARY_DIR}/ailee_service
            --rust-prover ${CMAKE_BINARY_DIR}/libailee_rust_prover.a
    DEPENDS generate_metadata ailee_node ailee_service
)

add_executable(ailee_node ${CORE_SOURCES})
add_dependencies(ailee_node generate_metadata_hpp)
target_link_libraries(ailee_node PRIVATE ailee_adapters ailee_l5_compression)

# --- Layer-2 Service Executable ---
add_executable(ailee_service
    src/integration/ailee_service.cpp
    ${AILEE_L2_SOURCES}
)
add_dependencies(ailee_service generate_metadata_hpp)

target_link_libraries(ailee_service
    PRIVATE
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${libsecp256k1_LIBRARIES}
)
if(nlohmann_json_FOUND)
    target_link_libraries(ailee_service PRIVATE nlohmann_json::nlohmann_json)
endif()

target_link_libraries(ailee_node
    PRIVATE
    ${libsecp256k1_LIBRARIES}
    $<IF:$<BOOL:${AILEE_USE_RUST_PROVER}>,ailee_rust_prover,ailee_rust_ffi_fallback>
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${libsecp256k1_LIBRARIES}
    Threads::Threads
)

target_compile_definitions(ailee_node PRIVATE
    AILEE_COMMIT_HASH=\"${AILEE_COMMIT_HASH}\"
    AILEE_BUILD_NUMBER=\"${AILEE_BUILD_NUMBER}\"
    AILEE_PROTOCOL_VERSION=\"${AILEE_PROTOCOL_VERSION}\"
)

if(jsoncpp_FOUND)
    target_link_libraries(ailee_node PRIVATE jsoncpp_lib)
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_JSONCPP=1)
endif()

# --- 2. L2 Verification Tool ---
add_executable(ailee_l2_verify
    src/tools/ailee_l2_verify.cpp
    src/l2/DeterministicEngine.cpp
    src/l2/EpochScheduler.cpp
    src/l2/ExecutionContext.cpp
    src/l2/RecoveryPayload.cpp
    src/l2/StateRootPipeline.cpp
    src/ReflectionLayer.cpp
    src/MeshCoherence.cpp
    src/NodeIdentity.cpp
    src/security/zk_proofs.cpp
    src/l3/NetworkReflection.cpp
    src/l3/NetworkBinding.cpp
    src/l3/GossipLayer.cpp
    src/l3/PeerSync.cpp
    src/l3/NetworkDriver.cpp
    src/l1/AnchorCommitment.cpp
    src/l2/L2State.cpp
)

target_link_libraries(ailee_l2_verify
    PRIVATE
    ${CURL_LIBRARIES}
    jsoncpp_lib
    ${libsecp256k1_LIBRARIES}
    $<IF:$<BOOL:${AILEE_USE_RUST_PROVER}>,ailee_rust_prover,ailee_rust_ffi_fallback>
    OpenSSL::SSL
    OpenSSL::Crypto
    ${libsecp256k1_LIBRARIES}
)

# Link yaml-cpp if found (for policy system)
if(yaml-cpp_FOUND)
    target_link_libraries(ailee_node PRIVATE yaml-cpp)
else()
    message(WARNING "yaml-cpp not found - policy hot-reload system will be disabled")
endif()

# Link RocksDB if found (for persistent storage)
if(RocksDB_FOUND)
    target_link_libraries(ailee_node PRIVATE ${ROCKSDB_LINK_LIBS})
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_ROCKSDB=1)
    message(STATUS "RocksDB support enabled (Libs: ${ROCKSDB_LINK_LIBS})")
else()
    message(WARNING "RocksDB not found - persistent storage will be disabled")
endif()

# Link libp2p if found (for P2P networking)
if(LIBP2P_FOUND)
    target_link_libraries(ailee_node PRIVATE ${LIBP2P_LIBRARIES})
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_LIBP2P=1)
    message(STATUS "libp2p support enabled")
else()
    message(WARNING "libp2p not found - P2P networking will use stub implementation")
endif()

# Robust ZeroMQ Linking
if(TARGET cppzmq)
    target_link_libraries(ailee_node PRIVATE cppzmq)
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_ZMQ=1)
elseif(ZeroMQ_LIBRARY)
    # Fallback if the target doesn't exist (manual linking)
    target_link_libraries(ailee_node PRIVATE ${ZeroMQ_LIBRARY})
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_ZMQ=1)
else()
    message(WARNING "ZeroMQ not found - real-time Bitcoin event listening will be disabled")
endif()

# --- 2. Adapter Library (Static) ---
# Useful for testing adapters without running the full node
add_library(ailee_adapters STATIC 
    src/l1_sync/mainnet_sync.cpp
    src/l1_sync/reorg_detector.cpp
    src/l1_sync/bitcoin_clock.cpp
    ${ADAPTER_SOURCES}
    ${AMBIENT_NODE_SOURCES}
    ${AMBIENT_MESH_SOURCES}
    ${PARTICIPATION_SOURCES}
    ${CONSENSUS_SOURCES}
    ${RUNTIME_SOURCES}
    ${PROTOCOL_SOURCES}
    ${ENERGY_SOURCES}
    ${AMBIENT_SOURCES_2}
    ${ANCHORING_SOURCES}
    ${IDENTITY_SOURCES}
    ${AMBIENT_SOURCES}
    ${WNN_SOURCES}
    ${AILEE_CORE_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${POLICY_SOURCES}
    src/security/zk_proofs.cpp
    src/l3/NetworkReflection.cpp
    src/l3/NetworkBinding.cpp
    src/l3/GossipLayer.cpp
    src/l3/PeerSync.cpp
    src/l3/NetworkDriver.cpp
    src/l1/AnchorCommitment.cpp
    src/l4/ClusterSim.cpp
    src/l4/DeterministicScheduler.cpp
    src/l4/RecoveryCoordinator.cpp
    src/l4/MeshAnchor.cpp
    src/l4/StateRootPropagation.cpp
    src/l4/DeterministicTransport.cpp
    src/l4/DeterministicTelemetry.cpp
    src/l4/DeterministicDashboard.cpp
    src/l4/ReplayBuffer.cpp
    src/l4/ReplayEngine.cpp
    src/l4/FederationConfig.cpp
    src/l4/ClusterFederationView.cpp
    src/l4/MultiClusterSim.cpp
    src/l6/StateRootBuilder.cpp
    src/l4/ReplayTick.cpp
    src/l6/ExternalSchema.cpp
    src/l6/JsonBindings.cpp
    src/l6/ReplayExport.cpp
    src/l6/FederationExport.cpp
    src/l6/AnchorSerializer.cpp
    src/l6/AnchorVerifier.cpp
)

add_library(ailee_l5_compression STATIC
    src/l5/CompressionConfig.cpp
    src/l5/DeterministicCompressor.cpp
)
target_include_directories(ailee_l5_compression PUBLIC include)

# ailee_adapters includes ReplayTick which is needed by compression
target_link_libraries(ailee_l5_compression PRIVATE ailee_adapters)

target_link_libraries(ailee_adapters
    PRIVATE
    $<IF:$<BOOL:${AILEE_USE_RUST_PROVER}>,ailee_rust_prover,ailee_rust_ffi_fallback>
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${libsecp256k1_LIBRARIES}
    Threads::Threads
)

target_compile_definitions(ailee_adapters PRIVATE
    AILEE_COMMIT_HASH=\"${AILEE_COMMIT_HASH}\"
    AILEE_BUILD_NUMBER=\"${AILEE_BUILD_NUMBER}\"
    AILEE_PROTOCOL_VERSION=\"${AILEE_PROTOCOL_VERSION}\"
)

if(jsoncpp_FOUND)
    target_link_libraries(ailee_adapters PRIVATE jsoncpp_lib)
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_JSONCPP=1)
endif()

# Link nlohmann/json if available (header-only)
if(nlohmann_json_FOUND)
    target_link_libraries(ailee_adapters PRIVATE nlohmann_json::nlohmann_json)
endif()

# Link yaml-cpp to library as well
if(yaml-cpp_FOUND)
    target_link_libraries(ailee_adapters PRIVATE yaml-cpp)
endif()

# Link RocksDB to library as well
if(RocksDB_FOUND)
    target_link_libraries(ailee_adapters PRIVATE ${ROCKSDB_LINK_LIBS})
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_ROCKSDB=1)
endif()

# Link libp2p to library as well
if(LIBP2P_FOUND)
    target_link_libraries(ailee_adapters PRIVATE ${LIBP2P_LIBRARIES})
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_LIBP2P=1)
endif()

# Robust ZeroMQ Linking for library
if(TARGET cppzmq)
    target_link_libraries(ailee_adapters PRIVATE cppzmq)
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_ZMQ=1)
elseif(ZeroMQ_LIBRARY)
    target_link_libraries(ailee_adapters PRIVATE ${ZeroMQ_LIBRARY})
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_ZMQ=1)
endif()

# ============================================================================
# Bitcoin Write Safety Gate
# ============================================================================

option(AILEE_BITCOIN_WRITE_ENABLED "Enable Bitcoin write operations (sendrawtransaction, sign, etc.)" OFF)
if(NOT AILEE_BITCOIN_WRITE_ENABLED)
    target_compile_definitions(ailee_node PRIVATE AILEE_BITCOIN_WRITE_DISABLED=1)
    target_compile_definitions(ailee_adapters PRIVATE AILEE_BITCOIN_WRITE_DISABLED=1)
    message(STATUS "Bitcoin write operations: DISABLED (shadow/read-only mode)")
else()
    message(STATUS "Bitcoin write operations: ENABLED")
endif()

# ============================================================================
# Unit Tests (Optional)
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        add_library(minigtest STATIC
            tests/minigtest/minigtest.cpp
        )
        target_include_directories(minigtest PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/minigtest
        )

        add_library(minigtest_main STATIC
            tests/minigtest/minigtest_main.cpp
        )
        target_link_libraries(minigtest_main PUBLIC minigtest)

        add_library(GTest::gtest ALIAS minigtest)
        add_library(GTest::gtest_main ALIAS minigtest_main)
    endif()
    enable_testing()

    add_executable(ailee_tests EXCLUDE_FROM_ALL
        tests/NodeIdentityTests.cpp
        tests/AdapterRegistryTests.cpp
        tests/BitVMInterpreterTests.cpp
        tests/MeshCoherenceTests.cpp
        tests/ReflectionLayerTests.cpp
        tests/DeterministicEngineTests.cpp
        tests/NetworkIntegrationTests.cpp
        tests/l3/GossipLayerTests.cpp
        tests/l3/PeerSyncTests.cpp
        tests/l4/test_cluster_sim.cpp
        tests/l4/test_deterministic_scheduler.cpp
        tests/l4/test_recovery_coordinator.cpp
        tests/l4/test_mesh_anchor.cpp
        tests/l4/test_state_root_propagation.cpp
        tests/l4/test_deterministic_transport.cpp
        tests/l4/test_deterministic_telemetry.cpp
        tests/l4/test_deterministic_dashboard.cpp
        tests/l4/test_deterministic_replay.cpp
        tests/l4/test_multi_cluster_sim.cpp
        tests/l5/test_deterministic_compression.cpp
        tests/l6/test_external_bindings.cpp
        tests/l6/test_anchor_serializer.cpp
        tests/l6/test_anchor_verifier.cpp
    )
    
    target_link_libraries(ailee_tests
        PRIVATE
        ailee_adapters
        ailee_l5_compression
        GTest::gtest
        GTest::gtest_main
    )
    
    add_test(NAME AdapterTests COMMAND ailee_tests)
    
    # ReorgDetector tests (requires RocksDB)
    if(RocksDB_FOUND)
        add_executable(reorg_detector_tests 
            tests/ReorgDetectorTests.cpp
            tests/SettlementIngestionTests.cpp
            src/l1/ReorgDetector.cpp
            src/l1/SettlementIngestion.cpp
        )
        
        target_include_directories(reorg_detector_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/minigtest
        )
        
        target_link_libraries(reorg_detector_tests
            PRIVATE
            ${ROCKSDB_LINK_LIBS}
            GTest::gtest
            GTest::gtest_main
        )
        
        target_compile_definitions(reorg_detector_tests PRIVATE AILEE_HAS_ROCKSDB=1)
        add_test(NAME ReorgDetectorTests COMMAND reorg_detector_tests)
    endif()
    
    # Optional: Add policy system tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/PolicyTests.cpp")
        add_executable(policy_tests tests/PolicyTests.cpp)
        target_link_libraries(policy_tests
            PRIVATE
            ailee_adapters
            GTest::gtest
            GTest::gtest_main
        )
        add_test(NAME PolicyTests COMMAND policy_tests)
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

# Install the binaries
install(TARGETS ailee_node DESTINATION bin)

# Install the Library Headers (So others can build on AILEE)
install(FILES 
    ${AILEE_HEADERS}
    ${POLICY_HEADERS}
    DESTINATION include/ailee
)

# Install default config file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml")
    install(FILES config.yaml DESTINATION etc/ailee)
endif()

# ============================================================================
# Post-Build: Create output directories
# ============================================================================

# Create directories for policy system outputs
add_custom_command(TARGET ailee_node POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        ${CMAKE_BINARY_DIR}/out
    COMMENT "Creating output directories for metrics"
)

# ============================================================================
# Package Configuration (Output Info)
# ============================================================================

message(STATUS "=============================================================")
message(STATUS "   AILEE Core v${PROJECT_VERSION} Build Configuration        ")
message(STATUS "=============================================================")
message(STATUS "   Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "   C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "   Optimization:     ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "-------------------------------------------------------------")
message(STATUS "   OpenSSL:          ${OPENSSL_VERSION}")
message(STATUS "   CURL:             ${CURL_VERSION_STRING}")
message(STATUS "   ZeroMQ:           ${ZeroMQ_FOUND}")
if(ZeroMQ_FOUND)
    message(STATUS "     - Include:      ${ZeroMQ_INCLUDE_DIR}")
    message(STATUS "     - Library:      ${ZeroMQ_LIBRARY}")
endif()
message(STATUS "   JsonCpp:          ${jsoncpp_FOUND}")
message(STATUS "   yaml-cpp:         ${yaml-cpp_FOUND}")
message(STATUS "-------------------------------------------------------------")
message(STATUS "   Core Modules:     TPS Engine, Recovery, Gold Bridge")
message(STATUS "   Network:          Sidechain Bridge, ZMQ Listener")
message(STATUS "   Governance:       DAO, Circuit Breaker")
message(STATUS "   Multi-Chain:      Global Seven (7+ adapters)")
message(STATUS "   AmbientAI:        Mesh Intelligence (Federated)")
message(STATUS "   Policy System:    ${yaml-cpp_FOUND}")
message(STATUS "   Web Server:       Enabled (port 8080)")
message(STATUS "   Build Tests:      ${BUILD_TESTS}")
message(STATUS "=============================================================")

# Warnings for missing optional dependencies
if(NOT yaml-cpp_FOUND)
    message(STATUS "")
    message(STATUS "⚠️  WARNING: yaml-cpp not found")
    message(STATUS "    Policy hot-reload system will be disabled")
    message(STATUS "    Install: sudo apt-get install libyaml-cpp-dev")
    message(STATUS "")
endif()

if(NOT ZeroMQ_FOUND)
    message(STATUS "")
    message(STATUS "⚠️  WARNING: ZeroMQ not found")
    message(STATUS "    Real-time Bitcoin monitoring will fall back to polling")
    message(STATUS "    Install: sudo apt-get install libzmq3-dev libcppzmq-dev")
    message(STATUS "")
endif()

# Add fallback FFI implementation so the codebase compiles
if(NOT AILEE_USE_RUST_PROVER)
    add_library(ailee_rust_ffi_fallback STATIC src/security/ailee_rust_ffi_fallback.cpp)
    target_include_directories(ailee_rust_ffi_fallback PUBLIC include)
endif()
