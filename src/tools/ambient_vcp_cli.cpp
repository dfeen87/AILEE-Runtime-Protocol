// SPDX-License-Identifier: MIT
// ambient_vcp_cli.cpp — Ambient VCP command-line interface
//
// Maintains node session state locally (via LocalSessionManager) even when
// disconnected from the API endpoint.  An observability server shares the
// *live* node pointer rather than a stale copy, so monitoring always
// reflects current state.
//
// When --endpoint is provided the CLI sends a real HTTP GET to the node's
// /api/health endpoint on every tick to confirm the hardware is reachable:
//   • node responds (2xx)  → setConnected(true)  — normal operation
//   • node unreachable     → setConnected(false) — offline-keepalive kicks in
// Transition events are logged to the session activity log.

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <string>

// POSIX socket headers for the HTTP health probe
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "../../include/AmbientAI.h"

using namespace ambient;

namespace {
    std::atomic<bool> g_running{true};

    void signalHandler(int) {
        g_running = false;
    }

    // Parse "http://host:port" or "http://host:port/path" → host + port.
    // Returns false if the URL cannot be parsed.
    bool parseEndpoint(const std::string& url,
                       std::string& host,
                       int& port) {
        std::string rest = url;
        // Strip scheme
        auto schemeEnd = rest.find("://");
        if (schemeEnd != std::string::npos) {
            rest = rest.substr(schemeEnd + 3);
        }
        // Strip path
        auto slashPos = rest.find('/');
        if (slashPos != std::string::npos) {
            rest = rest.substr(0, slashPos);
        }
        // Split host:port
        auto colonPos = rest.rfind(':');
        if (colonPos != std::string::npos) {
            host = rest.substr(0, colonPos);
            try {
                port = std::stoi(rest.substr(colonPos + 1));
            } catch (...) {
                return false;
            }
        } else {
            host = rest;
            port = 80;
        }
        return !host.empty() && port > 0;
    }

    // Send a real HTTP GET /api/health to the node hardware and check for 2xx.
    // This "hits" the hardware so the task-to-node connection is proven alive —
    // not just a TCP handshake, but an actual application-level health check.
    bool probeHardwareHealth(const std::string& host, int port) {
        addrinfo hints{};
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* res = nullptr;
        const std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
            return false;
        }

        int fd = -1;
        for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
            fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (fd < 0) continue;

            timeval tv{2, 0};   // 2-second timeout
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                       reinterpret_cast<const char*>(&tv), sizeof(tv));
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&tv), sizeof(tv));

            if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
            ::close(fd);
            fd = -1;
        }
        freeaddrinfo(res);
        if (fd < 0) return false;

        // Send HTTP GET /api/health
        const std::string req =
            "GET /api/health HTTP/1.0\r\n"
            "Host: " + host + "\r\n"
            "Connection: close\r\n"
            "\r\n";
        if (::send(fd, req.c_str(), req.size(), 0) < 0) {
            ::close(fd);
            return false;
        }

        // Read just the status line to check for 2xx
        char buf[64] = {};
        ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
        ::close(fd);
        if (n <= 0) return false;

        // "HTTP/1.x 2xx ..."
        std::string statusLine(buf, n);
        auto spacePos = statusLine.find(' ');
        if (spacePos == std::string::npos) return false;
        int statusCode = 0;
        try {
            statusCode = std::stoi(statusLine.substr(spacePos + 1, 3));
        } catch (...) {
            return false;
        }
        return statusCode >= 200 && statusCode < 300;
    }
} // namespace

// ---------------------------------------------------------------------------
// ObservabilityServer
//
// Shares the *live* AmbientNode via shared_ptr so it always reflects the
// current state.
// ---------------------------------------------------------------------------

class ObservabilityServer {
public:
    explicit ObservabilityServer(std::shared_ptr<AmbientNode> node)
        : node_(std::move(node)) {}

    void printStatus() const {
        const auto state = node_->sessionManager().getState();
        std::cout << "[observability]"
                  << " node="        << state.nodeId
                  << " connected="   << (state.connected ? "yes" : "no")
                  << " token="       << state.sessionToken
                  << " log_entries=" << state.activityLog.size()
                  << "\n";
    }

private:
    std::shared_ptr<AmbientNode> node_;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void printUsage(const char* exe) {
    std::cout << "Usage: " << exe << " --node-id <id> [--endpoint <url>] [--interval <secs>]\n"
              << "\n"
              << "  Maintains ambient VCP node session state locally,\n"
              << "  even when the node hardware is offline.\n"
              << "\n"
              << "Options:\n"
              << "  --node-id <id>      Node public key / identifier (required)\n"
              << "  --endpoint <url>    Base URL of the node hardware to probe,\n"
              << "                      e.g. http://192.168.1.42:8080\n"
              << "                      The CLI will GET /api/health on every tick.\n"
              << "                      When omitted the session is always kept alive\n"
              << "                      but connectivity is not tracked.\n"
              << "  --interval <secs>   Maintenance tick interval in seconds (default: 30)\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    std::string nodeId;
    std::string endpointUrl;
    int intervalSecs = 30;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--node-id" && i + 1 < argc) {
            nodeId = argv[++i];
        } else if (arg == "--endpoint" && i + 1 < argc) {
            endpointUrl = argv[++i];
        } else if (arg == "--interval" && i + 1 < argc) {
            try {
                intervalSecs = std::stoi(argv[++i]);
                if (intervalSecs <= 0) throw std::out_of_range("interval must be > 0");
            } catch (const std::exception& e) {
                std::cerr << "Error: --interval requires a positive integer (" << e.what() << ")\n";
                return 1;
            }
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (nodeId.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    // Parse endpoint URL into host + port for the hardware probe.
    std::string probeHost;
    int probePort = 0;
    bool probeEnabled = false;
    if (!endpointUrl.empty()) {
        if (!parseEndpoint(endpointUrl, probeHost, probePort)) {
            std::cerr << "Error: could not parse --endpoint URL: " << endpointUrl << "\n";
            return 1;
        }
        probeEnabled = true;
        std::cout << "[ambient-vcp-cli] Hardware probe: "
                  << probeHost << ":" << probePort << "/api/health\n";
    }

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    NodeId id;
    id.pubkey      = nodeId;
    id.region      = "local";
    id.deviceClass = "cli";

    SafetyPolicy policy;
    // node is a shared_ptr so both the maintenance loop and the observability
    // server operate on the *same* live object.
    uint64_t startupTimeMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    auto node = std::make_shared<AmbientNode>(id, policy, nullptr, startupTimeMs);
    node->sessionManager().recordActivity("[startup] ambient-vcp-cli started", startupTimeMs);

    // When an endpoint is given, start as disconnected so the first
    // successful probe fires the "reconnected" transition log entry.
    if (probeEnabled) {
        node->sessionManager().setConnected(false);
    }

    ObservabilityServer observability(node);

    std::cout << "[ambient-vcp-cli] Starting maintenance loop for node: " << nodeId << "\n";
    std::cout << "[ambient-vcp-cli] Maintenance interval: " << intervalSecs << "s\n";

    // Maintenance loop — on each tick:
    //   1. If --endpoint was given, GET /api/health on the node hardware.
    //      Log and print a message on each state transition.
    //   2. runMaintenanceTick() bumps the session token and, when disconnected,
    //      appends [offline-keepalive] so the session stays alive even while
    //      the node hardware is unreachable.
    bool prevConnected = false;
    while (g_running) {
        uint64_t loopTimestampMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        if (probeEnabled) {
            bool nowConnected = probeHardwareHealth(probeHost, probePort);

            if (nowConnected && !prevConnected) {
                node->sessionManager().setConnected(true);
                node->sessionManager().recordActivity(
                    "[reconnected] hardware at " + endpointUrl + " is online", loopTimestampMs);
                std::cout << "[ambient-vcp-cli] Node hardware online — session reconnected.\n";
            } else if (!nowConnected && prevConnected) {
                node->sessionManager().setConnected(false);
                node->sessionManager().recordActivity(
                    "[offline] hardware at " + endpointUrl + " unreachable — keepalive active", loopTimestampMs);
                std::cout << "[ambient-vcp-cli] Node hardware offline — keepalive active.\n";
            }

            prevConnected = nowConnected;
        }

        node->sessionManager().runMaintenanceTick(loopTimestampMs);
        observability.printStatus();

        for (int s = 0; s < intervalSecs && g_running; ++s) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    uint64_t shutdownTimeMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    node->sessionManager().recordActivity("[shutdown] ambient-vcp-cli stopped", shutdownTimeMs);
    std::cout << "[ambient-vcp-cli] Maintenance loop stopped.\n";
    return 0;
}
