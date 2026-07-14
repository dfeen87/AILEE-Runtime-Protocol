#include "ProtocolFrame.hpp"

// ---------------------------------------------------------
// Serialize ProtocolFrame → JSON string
// ---------------------------------------------------------
std::string serialize_frame(const ProtocolFrame& pf)
{
    Json::Value root;
    root["frame_id"]  = pf.frame_id;
    root["type"]      = pf.type;
    root["version"]   = pf.version;
    root["node_id"]   = pf.node_id;
    root["timestamp"] = (Json::UInt64) pf.timestamp;
    root["payload"]   = pf.payload;
    root["signature"] = pf.signature;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = ""; // compact for network transport
    return Json::writeString(builder, root);
}
