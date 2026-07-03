#include "ailee_api.hpp"

// Internal implementation of context
struct AileeContextImpl {
    ailee::AileeConfig config;
};

namespace ailee {

AileeContext ailee_init(const AileeConfig& config) {
    AileeContext ctx;
    ctx.impl = new AileeContextImpl();
    ctx.impl->config = config;
    return ctx;
}

void ailee_shutdown(AileeContext& context) {
    if (context.impl) {
        delete context.impl;
        context.impl = nullptr;
    }
}

} // namespace ailee
