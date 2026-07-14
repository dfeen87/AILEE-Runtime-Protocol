// SPDX-License-Identifier: MIT
// ProverSwarm.h — Deterministic Multi-Prover Swarm (Ambient AI v2)

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace rocksdb {
    class DB;
    class ColumnFamilyHandle;
}

namespace ailee::orchestration {

struct ProverIdentity {
    std::string pubkey;
    double reputation = 0.5;
    double capacity = 1.0;
    double latency_ms = 0.0;
    bool banned = false;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["pubkey"] = pubkey;
        j["reputation"] = reputation;
        j["capacity"] = capacity;
        j["latency_ms"] = latency_ms;
        j["banned"] = banned;
        return j;
    }

    static ProverIdentity fromJson(const nlohmann::json& j) {
        ProverIdentity p;
        if (j.contains("pubkey")) p.pubkey = j["pubkey"].get<std::string>();
        if (j.contains("reputation")) p.reputation = j["reputation"].get<double>();
        if (j.contains("capacity")) p.capacity = j["capacity"].get<double>();
        if (j.contains("latency_ms")) p.latency_ms = j["latency_ms"].get<double>();
        if (j.contains("banned")) p.banned = j["banned"].get<bool>();
        return p;
    }
};

struct ProverJob {
    std::string job_id;
    std::string payload;
    std::string assigned_prover;
    std::uint64_t assigned_at_ms = 0;
    bool completed = false;
    std::uint32_t retry_count = 0;

    nlohmann::json toJson() const {
        nlohmann::json j;

        j["job_id"] = job_id;
        j["payload"] = payload;
        j["assigned_prover"] = assigned_prover;

        // The json type in this build only has unambiguous operator= overloads
        // for signed integers and double, so an unsigned value (uint64_t/uint32_t)
        // matches more than one of them equally well. Casting to a signed
        // integer type first resolves to a single, unambiguous overload.
        j["assigned_at_ms"] = static_cast<std::int64_t>(assigned_at_ms);
        j["completed"] = completed;
        j["retry_count"] = static_cast<std::int64_t>(retry_count);

        return j;
    }

    static ProverJob fromJson(const nlohmann::json& j) {
        ProverJob job;

        if (j.contains("job_id"))
            job.job_id = j["job_id"].get<std::string>();

        if (j.contains("payload"))
            job.payload = j["payload"].get<std::string>();

        if (j.contains("assigned_prover"))
            job.assigned_prover = j["assigned_prover"].get<std::string>();

        if (j.contains("assigned_at_ms"))
            job.assigned_at_ms = static_cast<std::uint64_t>(j["assigned_at_ms"].get<std::int64_t>());

        if (j.contains("completed"))
            job.completed = j["completed"].get<bool>();

        if (j.contains("retry_count"))
            job.retry_count = static_cast<std::uint32_t>(j["retry_count"].get<std::int64_t>());

        return job;
    }
};

struct SwarmMetrics {
    std::uint64_t queue_depth = 0;
    double avg_proof_latency_ms = 0.0;
    std::uint64_t completed_jobs = 0;
    std::uint64_t failed_jobs = 0;
    std::uint64_t active_provers = 0;
};

struct ProverSwarmConfig {
    double alpha = 0.05;
    double beta = 0.10;
    std::uint64_t timeout_ms = 5000;
    std::string db_path = "./swarm_db";
    std::string jobs_cf_name = "prover_jobs_cf";
    std::string state_cf_name = "prover_state_cf";
};

class ProverSwarm {
public:
    explicit ProverSwarm(const ProverSwarmConfig& config);
    ~ProverSwarm();

    bool initialize(std::string* err = nullptr);
    void close();

    bool submitJob(const std::string& job_id, const std::string& payload_json, std::string* err = nullptr);
    std::optional<ProverJob> assignJob();
    std::optional<ProverJob> getJob(const std::string& job_id) const;

    bool recordJobSuccess(const std::string& job_id, double latency_ms, std::string* err = nullptr);
    bool recordJobFailure(const std::string& job_id, std::string* err = nullptr);
    bool hardBanProver(const std::string& pubkey, const std::string& reason, std::string* err = nullptr);

    bool registerProver(const ProverIdentity& prover, std::string* err = nullptr);

    std::vector<std::string> checkTimeouts(std::uint64_t current_time_ms);

    std::optional<ProverIdentity> getProver(const std::string& pubkey) const;
    std::vector<ProverIdentity> getAllProvers() const;
    SwarmMetrics getMetrics() const;
    std::vector<std::string> getQueueOrder() const;

private:
    ProverSwarmConfig config_;

    std::unique_ptr<rocksdb::DB> db_;
    rocksdb::ColumnFamilyHandle* default_cf_ = nullptr;
    rocksdb::ColumnFamilyHandle* jobs_cf_ = nullptr;
    rocksdb::ColumnFamilyHandle* state_cf_ = nullptr;

    mutable std::mutex mu_;

    std::vector<std::string> queue_order_;

    SwarmMetrics metrics_;
    double total_latency_ms_ = 0.0;

    bool saveQueueOrder(std::string* err = nullptr);
    bool loadQueueOrder(std::string* err = nullptr);
    bool saveJob(const ProverJob& job, std::string* err = nullptr);
    bool saveProver(const ProverIdentity& prover, std::string* err = nullptr);

    std::optional<ProverJob> fetchJobInternal(const std::string& job_id) const;
    std::optional<ProverIdentity> fetchProverInternal(const std::string& pubkey) const;
};

} // namespace ailee::orchestration
