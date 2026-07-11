#include "l6/ZKBackendFactory.h"
#include "l6/ZKMockBackend.h"
#include "l6/Halo2Backend.h"
#include "l6/PlonkBackend.h"
#include "Logging.h"

namespace ailee::l6 {

std::unique_ptr<IZKProvingBackend> make_backend(const ZKBackendConfig& config) {
    auto logger = ailee::log::getLogger("L6");

    if (config.circuit_path.empty() || config.proving_key_path.empty() || config.verification_key_path.empty()) {
        LOG_WARN(logger, "ZK backend: One or more key paths are empty (circuit, proving, or verification)");
    }

    switch (config.type) {
    case ZKBackendType::MOCK:
        LOG_INFO(logger, "ZK backend: MOCK (deterministic)");
        return std::make_unique<ZKMockBackend>();
    case ZKBackendType::HALO2_NATIVE:
        LOG_WARN(logger, "ZK backend: HALO2_NATIVE (non-deterministic stub)");
        return std::make_unique<Halo2Backend>();
    case ZKBackendType::PLONK_NATIVE:
        LOG_WARN(logger, "ZK backend: PLONK_NATIVE (non-deterministic stub)");
        return std::make_unique<PlonkBackend>();
    default:
        LOG_INFO(logger, "ZK backend: MOCK (deterministic)");
        return std::make_unique<ZKMockBackend>();
    }
}

} // namespace ailee::l6
