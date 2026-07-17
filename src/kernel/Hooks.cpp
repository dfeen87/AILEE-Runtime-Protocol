#include "kernel/Hooks.h"
#include "webserver/RouteRegistry.h"
#include "l6/ExternalSchema.h"
#include "l6/JsonBindings.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace ailee {
namespace kernel {

class AuditTrailWorker {
public:
    static AuditTrailWorker& instance() {
        static AuditTrailWorker worker;
        return worker;
    }

    void enqueueTick(const std::string& tick_json_str) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            // Bounded queue: limit to 1000 items to avoid infinite memory growth if I/O hangs
            if (queue_.size() < 1000) {
                queue_.push(tick_json_str);
            }
        }
        cv_.notify_one();
    }

    ~AuditTrailWorker() {
        stop();
    }

private:
    AuditTrailWorker() : running_(true) {
        worker_thread_ = std::thread(&AuditTrailWorker::processQueue, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            running_ = false;
        }
        cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    void processQueue() {
        while (true) {
            std::string tick_json_str;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this]() { return !queue_.empty() || !running_; });

                if (queue_.empty() && !running_) {
                    break;
                }

                tick_json_str = std::move(queue_.front());
                queue_.pop();
            }

            writeTickToAuditTrail(tick_json_str);
        }
    }

    void writeTickToAuditTrail(const std::string& tick_json_str) {
        try {
            std::filesystem::create_directories("logs");

            std::string filepath = "logs/audit_trail.json";
            std::string tmp_filepath = "logs/audit_trail.json.tmp";

            // Rotate if file is larger than 5MB
            if (std::filesystem::exists(filepath) && std::filesystem::file_size(filepath) >= 5 * 1024 * 1024) {
                rotateFiles(filepath);
            }

            nlohmann::json audit_list = nlohmann::json::array();
            std::ifstream infile(filepath);
            if (infile.is_open()) {
                std::string file_content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
                infile.close();
                if (!file_content.empty()) {
                    try {
                        audit_list = nlohmann::json::parse(file_content);
                    } catch (...) {
                        // corrupt fallback
                    }
                }
            }

            nlohmann::json tick_json = nlohmann::json::parse(tick_json_str);
            audit_list.push_back(tick_json);

            // Atomic write: write to tmp file first
            std::ofstream outfile(tmp_filepath, std::ios::trunc);
            if (outfile.is_open()) {
                outfile << audit_list.dump();
                outfile.flush();
                outfile.close();

                // Rename tmp file to target file atomically
                std::filesystem::rename(tmp_filepath, filepath);
            }
        } catch (...) {
            // Fail-safe
        }
    }

    void rotateFiles(const std::string& filepath) {
        try {
            // Keep up to 3 old files: audit_trail.json.3, audit_trail.json.2, audit_trail.json.1
            std::string file3 = filepath + ".3";
            std::string file2 = filepath + ".2";
            std::string file1 = filepath + ".1";

            if (std::filesystem::exists(file2)) {
                std::filesystem::rename(file2, file3);
            }
            if (std::filesystem::exists(file1)) {
                std::filesystem::rename(file1, file2);
            }
            if (std::filesystem::exists(filepath)) {
                std::filesystem::rename(filepath, file1);
            }
        } catch (...) {
            // ignore rotation errors
        }
    }

    std::thread worker_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::queue<std::string> queue_;
    std::atomic<bool> running_;
};

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
        std::string tick_json_str = l6::JsonBindings::to_json(tick);
        AuditTrailWorker::instance().enqueueTick(tick_json_str);
    } catch (...) {
        // fail-safe / no exception propagation
    }
}

} // namespace kernel
} // namespace ailee
