#include "kernel/Hooks.h"
#include "webserver/RouteRegistry.h"
#include "l6/ExternalSchema.h"
#include "l6/JsonBindings.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace ailee {
namespace kernel {

void Hooks::onRouteRegistered(const Route& route) {
    // Keep logs structured, minimal and clean
    std::cout << "[Kernel] Route registered: " << route.path
              << " [" << (route.method == HttpMethod::GET ? "GET" : "POST") << "]"
              << " (kernel_aware: " << (route.kernel_aware ? "true" : "false") << ")"
              << std::endl;
}

void Hooks::onStateSnapshotRequested(const StateSnapshotContext& ctx, l6::ExternalStateSnapshot& snapshot) {
    (void)ctx;
    (void)snapshot;
    // Early Kernel Attachment point - governance placeholder
}

void Hooks::onReplayTick(const ReplayTickContext& ctx, const l6::ExternalReplayTick& tick) {
    (void)ctx;
    try {
        std::filesystem::create_directories("logs");

        nlohmann::json audit_list = nlohmann::json::array();
        std::ifstream infile("logs/audit_trail.json");
        if (infile.is_open()) {
            std::string file_content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
            infile.close();
            if (!file_content.empty()) {
                try {
                    audit_list = nlohmann::json::parse(file_content);
                } catch (...) {
                    // fallback if corrupt/empty
                }
            }
        }

        std::string tick_json_str = l6::JsonBindings::to_json(tick);
        nlohmann::json tick_json = nlohmann::json::parse(tick_json_str);

        audit_list.push_back(tick_json);

        std::ofstream outfile("logs/audit_trail.json", std::ios::trunc);
        if (outfile.is_open()) {
            outfile << audit_list.dump();
            outfile.close();
        }
    } catch (...) {
        // fail-safe / no exception propagation
    }
}

} // namespace kernel
} // namespace ailee
