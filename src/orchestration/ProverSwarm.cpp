// SPDX-License-Identifier: MIT
// ProverSwarm.cpp — Deterministic Multi-Prover Swarm (Ambient AI v2)

#include "ProverSwarm.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <iostream>
#include <algorithm>

namespace ailee::orchestration {

ProverSwarm::ProverSwarm(const ProverSwarmConfig& config)
    : config_(config) {}

ProverSwarm::~ProverSwarm() {
    close();
}

bool ProverSwarm::initialize(std::string* err) {
    rocksdb::Options options;
    options.create_if_missing = true;

    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(config_.jobs_cf_name, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(config_.state_cf_name, rocksdb::ColumnFamilyOptions()));

    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, config_.db_path, column_families, &handles, &db_ptr);

    if (!status.ok()) {
        // If it failed because CFs don't exist, we must create them.
        rocksdb::Options create_opts = options;
        status = rocksdb::DB::Open(create_opts, config_.db_path, &db_ptr);
        if (!status.ok()) {
            if (err) *err = "Failed to open RocksDB for ProverSwarm: " + status.ToString();
            return false;
        }

        rocksdb::ColumnFamilyHandle* cf;
        status = db_ptr->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), config_.jobs_cf_name, &cf);
        if (status.ok()) {
            db_ptr->DestroyColumnFamilyHandle(cf);
        }
        status = db_ptr->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), config_.state_cf_name, &cf);
        if (status.ok()) {
            db_ptr->DestroyColumnFamilyHandle(cf);
        }
        delete db_ptr;

        // Re-open with CFs
        status = rocksdb::DB::Open(options, config_.db_path, column_families, &handles, &db_ptr);
        if (!status.ok()) {
            if (err) *err = "Failed to open RocksDB with CFs: " + status.ToString();
            return false;
        }
    }

    db_.reset(db_ptr);
    default_cf_ = handles[0];
    jobs_cf_ = handles[1];
    state_cf_ = handles[2];

    if (!loadQueueOrder(err)) {
        return false;
    }

    // Recover metrics
    std::lock_guard<std::mutex> lock(mu_);
    metrics_.queue_depth = queue_order_.size();

    return true;
}

void ProverSwarm::close() {
    if (db_) {
        std::lock_guard<std::mutex> lock(mu_);
        if (default_cf_) { db_->DestroyColumnFamilyHandle(default_cf_); default_cf_ = nullptr; }
        if (jobs_cf_) { db_->DestroyColumnFamilyHandle(jobs_cf_); jobs_cf_ = nullptr; }
        if (state_cf_) { db_->DestroyColumnFamilyHandle(state_cf_); state_cf_ = nullptr; }
        db_.reset();
    }
}

bool ProverSwarm::submitJob(const std::string& job_id, const std::string& payload_json, std::string* err) {
    std::lock_guard<std::mutex> lock(mu_);

    // Check if job exists
    if (fetchJobInternal(job_id).has_value()) {
        if (err) *err = "Job already exists.";
        return false;
    }

    ProverJob job;
    job.job_id = job_id;
    job.payload = payload_json;

    queue_order_.push_back(job_id);

    rocksdb::WriteBatch batch;

    // Serialize job
    std::string job_key = "jobs/" + job.job_id;
    std::string job_val = job.toJson().dump();
    batch.Put(jobs_cf_, job_key, job_val);

    // Serialize queue order
    nlohmann::json queue_j = nlohmann::json::array();
    for (size_t i = 0; i < queue_order_.size(); ++i) {
        queue_j[i] = queue_order_[i];
    }
    std::string queue_val = queue_j.dump();
    batch.Put(jobs_cf_, "queue_order", queue_val);

    rocksdb::Status s = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
        if (err) *err = "Failed to submit job: " + s.ToString();
        queue_order_.pop_back();
        return false;
    }

    metrics_.queue_depth = queue_order_.size();
    return true;
}

std::optional<ProverJob> ProverSwarm::assignJob() {
    std::lock_guard<std::mutex> lock(mu_);
    if (queue_order_.empty()) {
        return std::nullopt;
    }

    // 1. Find the highest scoring prover
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions(), state_cf_);
    ProverIdentity best_prover;
    double best_score = -1.0;
    bool found_prover = false;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        try {
            auto j = nlohmann::json::parse(it->value().ToString());
            ProverIdentity prover = ProverIdentity::fromJson(j);

            if (prover.banned) continue;

            // Simple heuristic for score: reputation + capacity - (latency / 1000)
            double score = prover.reputation + prover.capacity - (prover.latency_ms / 1000.0);
            if (score > best_score) {
                best_score = score;
                best_prover = prover;
                found_prover = true;
            }
        } catch (...) {
            // Ignore parse errors
        }
    }
    delete it;

    if (!found_prover) {
        return std::nullopt;
    }

    // 2. Find the first unassigned job in queue
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto& job_id : queue_order_) {
        auto job_opt = fetchJobInternal(job_id);
        if (!job_opt) continue;

        auto job = *job_opt;
        if (job.completed) continue;

        if (job.assigned_prover.empty()) {
            job.assigned_prover = best_prover.pubkey;
            job.assigned_at_ms = now;
            job.retry_count++;

            if (saveJob(job)) {
                return job;
            }
        }
    }

    return std::nullopt;
}

std::optional<ProverJob> ProverSwarm::getJob(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    return fetchJobInternal(job_id);
}

bool ProverSwarm::recordJobSuccess(const std::string& job_id, double latency_ms, std::string* err) {
    std::lock_guard<std::mutex> lock(mu_);
    auto job_opt = fetchJobInternal(job_id);
    if (!job_opt) {
        if (err) *err = "Job not found.";
        return false;
    }

    auto job = *job_opt;
    if (job.completed) {
        return true;
    }

    job.completed = true;

    rocksdb::WriteBatch batch;

    // Save job
    std::string job_key = "jobs/" + job.job_id;
    std::string job_val = job.toJson().dump();
    batch.Put(jobs_cf_, job_key, job_val);

    // Save prover
    auto prover_opt = fetchProverInternal(job.assigned_prover);
    if (prover_opt) {
        auto prover = *prover_opt;
        prover.reputation += config_.alpha / (1.0 + latency_ms);
        prover.latency_ms = (prover.latency_ms + latency_ms) / 2.0; // Moving avg
        batch.Put(state_cf_, prover.pubkey, prover.toJson().dump());
    }

    // Remove from queue
    auto it = std::find(queue_order_.begin(), queue_order_.end(), job_id);
    if (it != queue_order_.end()) {
        queue_order_.erase(it);

        nlohmann::json queue_j = nlohmann::json::array();
        for (size_t i = 0; i < queue_order_.size(); ++i) {
            queue_j[i] = queue_order_[i];
        }
        batch.Put(jobs_cf_, "queue_order", queue_j.dump());
    }

    rocksdb::Status s = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
        if (err) *err = "Failed to record job success: " + s.ToString();
        return false;
    }

    metrics_.completed_jobs++;
    metrics_.queue_depth = queue_order_.size();
    total_latency_ms_ += latency_ms;
    metrics_.avg_proof_latency_ms = (metrics_.completed_jobs > 0)
        ? (total_latency_ms_ / static_cast<double>(metrics_.completed_jobs))
        : 0.0;

    return true;
}

bool ProverSwarm::recordJobFailure(const std::string& job_id, std::string* err) {
    std::lock_guard<std::mutex> lock(mu_);
    auto job_opt = fetchJobInternal(job_id);
    if (!job_opt) {
        if (err) *err = "Job not found.";
        return false;
    }

    auto job = *job_opt;
    if (job.completed) return true;

    rocksdb::WriteBatch batch;

    auto prover_opt = fetchProverInternal(job.assigned_prover);
    if (prover_opt) {
        auto prover = *prover_opt;
        prover.reputation -= config_.beta;
        if (prover.reputation < 0) prover.reputation = 0;
        batch.Put(state_cf_, prover.pubkey, prover.toJson().dump());
    }

    // Clear assignment so it can be picked up again
    job.assigned_prover = "";
    job.assigned_at_ms = 0;

    std::string job_key = "jobs/" + job.job_id;
    batch.Put(jobs_cf_, job_key, job.toJson().dump());

    rocksdb::Status s = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
        if (err) *err = "Failed to record job failure: " + s.ToString();
        return false;
    }

    metrics_.failed_jobs++;
    return true;
}

bool ProverSwarm::hardBanProver(const std::string& pubkey, const std::string& reason, std::string* err) {
    (void)reason;
    std::lock_guard<std::mutex> lock(mu_);
    auto prover_opt = fetchProverInternal(pubkey);
    if (!prover_opt) {
        if (err) *err = "Prover not found.";
        return false;
    }

    auto prover = *prover_opt;
    prover.banned = true;
    prover.reputation = 0.0;
    return saveProver(prover, err);
}

bool ProverSwarm::registerProver(const ProverIdentity& prover, std::string* err) {
    std::lock_guard<std::mutex> lock(mu_);

    // Avoid overriding if already exists, maybe merge?
    auto existing = fetchProverInternal(prover.pubkey);
    if (existing) {
        // Just return true if it exists, or handle update logic
        return true;
    }

    if (saveProver(prover, err)) {
        metrics_.active_provers++;
        return true;
    }
    return false;
}

std::vector<std::string> ProverSwarm::checkTimeouts(std::uint64_t current_time_ms) {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<std::string> timed_out_jobs;
    rocksdb::WriteBatch batch;
    bool has_updates = false;

    for (const auto& job_id : queue_order_) {
        auto job_opt = fetchJobInternal(job_id);
        if (!job_opt) continue;

        auto job = *job_opt;
        if (job.completed || job.assigned_prover.empty()) continue;

        if (job.assigned_at_ms != 0 && current_time_ms > job.assigned_at_ms &&
            (current_time_ms - job.assigned_at_ms) > config_.timeout_ms) {
            timed_out_jobs.push_back(job_id);

            // Penalize the prover
            auto prover_opt = fetchProverInternal(job.assigned_prover);
            if (prover_opt) {
                auto prover = *prover_opt;
                prover.reputation -= config_.beta;
                if (prover.reputation < 0) prover.reputation = 0;
                batch.Put(state_cf_, prover.pubkey, prover.toJson().dump());
            }

            // Unassign
            job.assigned_prover = "";
            job.assigned_at_ms = 0;
            std::string job_key = "jobs/" + job.job_id;
            batch.Put(jobs_cf_, job_key, job.toJson().dump());

            has_updates = true;
        }
    }

    if (has_updates) {
        rocksdb::Status s = db_->Write(rocksdb::WriteOptions(), &batch);
        if (!s.ok()) {
            timed_out_jobs.clear();
        }
    }

    return timed_out_jobs;
}

std::optional<ProverIdentity> ProverSwarm::getProver(const std::string& pubkey) const {
    std::lock_guard<std::mutex> lock(mu_);
    return fetchProverInternal(pubkey);
}

std::vector<ProverIdentity> ProverSwarm::getAllProvers() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<ProverIdentity> provers;
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions(), state_cf_);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        try {
            auto j = nlohmann::json::parse(it->value().ToString());
            provers.push_back(ProverIdentity::fromJson(j));
        } catch (...) {}
    }
    delete it;
    return provers;
}

SwarmMetrics ProverSwarm::getMetrics() const {
    std::lock_guard<std::mutex> lock(mu_);
    return metrics_;
}

std::vector<std::string> ProverSwarm::getQueueOrder() const {
    std::lock_guard<std::mutex> lock(mu_);
    return queue_order_;
}

// ---------------------------------------------------------
// Internal persistence helpers
// ---------------------------------------------------------

bool ProverSwarm::saveQueueOrder(std::string* err) {
    nlohmann::json j = nlohmann::json::array();
    for (size_t i = 0; i < queue_order_.size(); ++i) {
        j[i] = queue_order_[i];
    }
    std::string val = j.dump();
    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), jobs_cf_, "queue_order", val);
    if (!s.ok()) {
        if (err) *err = "Failed to save queue_order: " + s.ToString();
        return false;
    }
    return true;
}

bool ProverSwarm::loadQueueOrder(std::string* err) {
    std::string val;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), jobs_cf_, "queue_order", &val);
    if (s.IsNotFound()) {
        queue_order_.clear();
        return true;
    }
    if (!s.ok()) {
        if (err) *err = "Failed to load queue_order: " + s.ToString();
        return false;
    }
    try {
        auto j = nlohmann::json::parse(val);
        queue_order_.clear();
        if (j.is_array()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                queue_order_.push_back((*it).get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        if (err) *err = "Failed to parse queue_order JSON: " + std::string(e.what());
        return false;
    }
    return true;
}

bool ProverSwarm::saveJob(const ProverJob& job, std::string* err) {
    std::string key = "jobs/" + job.job_id;
    std::string val = job.toJson().dump();
    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), jobs_cf_, key, val);
    if (!s.ok()) {
        if (err) *err = "Failed to save job " + job.job_id + ": " + s.ToString();
        return false;
    }
    return true;
}

bool ProverSwarm::saveProver(const ProverIdentity& prover, std::string* err) {
    std::string val = prover.toJson().dump();
    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), state_cf_, prover.pubkey, val);
    if (!s.ok()) {
        if (err) *err = "Failed to save prover " + prover.pubkey + ": " + s.ToString();
        return false;
    }
    return true;
}

std::optional<ProverJob> ProverSwarm::fetchJobInternal(const std::string& job_id) const {
    std::string val;
    std::string key = "jobs/" + job_id;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), jobs_cf_, key, &val);
    if (s.ok()) {
        try {
            auto j = nlohmann::json::parse(val);
            return ProverJob::fromJson(j);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<ProverIdentity> ProverSwarm::fetchProverInternal(const std::string& pubkey) const {
    std::string val;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), state_cf_, pubkey, &val);
    if (s.ok()) {
        try {
            auto j = nlohmann::json::parse(val);
            return ProverIdentity::fromJson(j);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace ailee::orchestration
