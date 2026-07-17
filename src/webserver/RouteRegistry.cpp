#include "webserver/RouteRegistry.h"
#include "kernel/Hooks.h"

namespace ailee {

RouteRegistry& RouteRegistry::getInstance() {
    static RouteRegistry instance;
    return instance;
}

void RouteRegistry::registerRoute(const Route& route) {
    for (const auto& r : routes_) {
        if (r.path == route.path && r.method == route.method) {
            return; // Already registered, prevent duplicates
        }
    }
    routes_.push_back(route);
    // Wire early kernel hook
    kernel::Hooks::onRouteRegistered(route);
}

const std::vector<Route>& RouteRegistry::getRoutes() const {
    return routes_;
}

} // namespace ailee
