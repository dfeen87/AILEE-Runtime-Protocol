#include "AmbientProtocol.hpp"
#include "ProtocolFrame.hpp"
#include <iostream>
#include <ctime>

// ---------------------------------------------------------
// Process inbound protocol frame
// ---------------------------------------------------------
void AmbientProtocol::processInboundFrame(const ProtocolFrame& frame)
{
    std::cout << "[AmbientProtocol] Processing inbound frame" << std::endl;
    std::cout << "  Type: "      << frame.type      << std::endl;
    std::cout << "  Version: "   << frame.version   << std::endl;
    std::cout << "  Node ID: "   << frame.node_id   << std::endl;
    std::cout << "  Timestamp: " << frame.timestamp << std::endl;
    std::cout << "  Payload: "   << frame.payload   << std::endl;
    std::cout << "  Signature: " << frame.signature << std::endl;

    // Placeholder: later this will forward to Orchestrator
    // for activation negotiation, swarm sync, etc.
}

// ---------------------------------------------------------
// Build protocol response (placeholder)
// ---------------------------------------------------------
Json::Value AmbientProtocol::buildProtocolResponse(const std::string& type)
{
    Json::Value response;
    response["response_type"] = type;
    response["status"] = "ok";

    // Correct timestamp syntax
    response["timestamp"] = (Json::UInt64) std::time(nullptr);

    return response;
}
