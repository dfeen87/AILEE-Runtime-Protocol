#include "ProtocolFrame.hpp"
#include <openssl/sha.h>  // TEMP: hash-based signature

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
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

// TEMP: hash-based “signature” placeholder.
// Later: replace with real secp256k1 ECDSA/Schnorr.
std::string sign_frame(const ProtocolFrame& pf)
{
    std::string data = pf.frame_id + pf.type + pf.version +
                       pf.node_id + std::to_string(pf.timestamp) +
                       pf.payload;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), hash);

    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(digits[(hash[i] >> 4) & 0xF]);
        hex.push_back(digits[hash[i] & 0xF]);
    }
    return hex;
}
