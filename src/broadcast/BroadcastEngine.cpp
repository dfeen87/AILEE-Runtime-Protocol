#include "BroadcastEngine.hpp"
#include <iostream>

void BroadcastEngine::emit(const std::string& type,
                           const std::string& version,
                           const Json::Value& payload)
{
    std::cout << "[BroadcastEngine] Emitting broadcast" << std::endl;
    std::cout << "  Type: " << type << std::endl;
    std::cout << "  Version: " << version << std::endl;
    std::cout << "  Payload: " << payload.toStyledString() << std::endl;
}
