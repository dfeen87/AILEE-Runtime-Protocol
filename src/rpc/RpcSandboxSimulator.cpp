#include "RpcSandboxSimulator.h"
#include <thread>
#include <chrono>

namespace ailee {

RpcSandboxSimulator::RpcSandboxSimulator()
    : simulate404_(false), latencyMs_(0), currentBlockCount_(800000) {}

void RpcSandboxSimulator::setSimulate404(bool enable) {
    simulate404_ = enable;
}

void RpcSandboxSimulator::setSimulatedLatency(int ms) {
    latencyMs_ = ms;
}

long RpcSandboxSimulator::execute(const std::string& method, const std::string& /*params*/, std::string& responseBuffer) {
    if (latencyMs_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(latencyMs_));
    }

    if (simulate404_) {
        // Return HTML for a 404 to mimic a real HTTP server responding with an error page
        responseBuffer = "<html><body>404 Not Found</body></html>";
        return 404;
    }

    nlohmann::json response;
    response["id"] = "ailee-core";
    response["error"] = nullptr;

    if (method == "getblockcount") {
        response["result"] = static_cast<uint64_t>(currentBlockCount_);
    } else if (method == "sendrawtransaction") {
        // Mock a successful txid
        response["result"] = "mock_txid_a1b2c3d4e5f6";
    } else if (method == "getrawtransaction") {
        nlohmann::json result;
        result["txid"] = "mock_txid";
        result["hash"] = "mock_hash";
        result["confirmations"] = 6;
        result["time_received"] = 1712345678; // Coercible if null in tests
        response["result"] = result;
    } else {
        response["error"] = {{"code", -32601}, {"message", "Method not found"}};
        response["result"] = nullptr;
    }

    responseBuffer = response.dump();
    return 200;
}

} // namespace ailee
