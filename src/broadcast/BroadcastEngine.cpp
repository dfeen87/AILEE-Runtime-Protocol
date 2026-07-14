#include "BroadcastEngine.hpp"
#include "NetworkBinding.hpp"
#include "ProtocolFrame.hpp"
#include <iostream>
#include <ctime>

// Static network binding pointer
NetworkBinding* BroadcastEngine::net = nullptr;

// TODO: replace with your real frame_id generator
static std::string generate_frame_id() {
    return "frame-local-placeholder";
}

void BroadcastEngine::bind(NetworkBinding* binding)
{
    net = binding;
    std::cout << "[BroadcastEngine] Network binding attached" << std::endl;
}

void BroadcastEngine::emit(const std::string& type,
                           const std::string& version,
                           const Json::Value& payload)
{
    if (!net) {
        std::cout << "[BroadcastEngine] No network binding available" << std::endl;
        std::cout << "  Type: " << type << std::endl;
        std::cout << "  Version: " << version << std::endl;
        std::cout << "  Payload: " << payload.toStyledString() << std::endl;
        return;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string serialized_payload = Json::writeString(builder, payload);

    ProtocolFrame pf;
    pf.frame_id  = generate_frame_id();
    pf.type      = type;
    pf.version   = version;
    pf.node_id   = "local-node";
    pf.timestamp = std::time(nullptr);
    pf.payload   = serialized_payload;

    // 🔐 sign the frame
    pf.signature = sign_frame(pf);

    // serialize full frame
    std::string serialized = serialize_frame(pf);

    // send to network
    net->broadcast(serialized);
}
