#pragma once

#include "l6/IslaRuntimeOrchestrator.h"
#include <string>

namespace ailee::l6 {

struct BitcoinRpcConfig {
    std::string url;
    std::string user;
    std::string pass;
};

class BitcoinClock : public IClock {
public:
    explicit BitcoinClock(const BitcoinRpcConfig& config);
    ~BitcoinClock() override;

    ClockSnapshot get_snapshot() const override;

private:
    BitcoinRpcConfig config_;

    // Internal helper to make JSON-RPC calls via libcurl
    std::string make_rpc_call(const std::string& method, const std::string& params) const;
};

} // namespace ailee::l6
