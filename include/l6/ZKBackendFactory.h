#pragma once

#include <memory>
#include "l6/ZKProvingBackend.h"

namespace ailee::l6 {

std::unique_ptr<IZKProvingBackend> make_backend(const ZKBackendConfig& config);

} // namespace ailee::l6
