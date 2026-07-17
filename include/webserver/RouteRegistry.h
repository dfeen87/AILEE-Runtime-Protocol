#pragma once

#include <string>
#include <vector>
#include <functional>
#include "third_party/httplib.h"

namespace ailee {

enum class HttpMethod {
    GET,
    POST
};

struct Route {
    std::string path;
    HttpMethod method;
    std::function<void(const httplib::Request&, httplib::Response&)> handler;
    std::string signature_metadata;
    bool kernel_aware = false;
};

class RouteRegistry {
public:
    static RouteRegistry& getInstance();

    void registerRoute(const Route& route);
    const std::vector<Route>& getRoutes() const;

private:
    RouteRegistry() = default;
    std::vector<Route> routes_;
};

} // namespace ailee
