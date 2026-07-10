#include "l6/ZKBackendFactory.h"
#include "l6/ZKMockBackend.h"
#include "l6/Halo2Backend.h"
#include "l6/PlonkBackend.h"

namespace ailee::l6 {

std::unique_ptr<IZKProvingBackend> make_backend(const ZKBackendConfig& config) {
    switch (config.type) {
    case ZKBackendType::MOCK:
        return std::make_unique<ZKMockBackend>();
    case ZKBackendType::HALO2_NATIVE:
        return std::make_unique<Halo2Backend>();
    case ZKBackendType::PLONK_NATIVE:
        return std::make_unique<PlonkBackend>();
    default:
        return std::make_unique<ZKMockBackend>();
    }
}

} // namespace ailee::l6
