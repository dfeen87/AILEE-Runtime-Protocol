// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// PrometheusExporter.cpp — Prometheus metrics export implementation

#include "PrometheusExporter.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ailee::metrics {

// ==================== Counter Implementation ====================

Counter::Counter(const std::string& name, const std::string& help,
                 const std::map<std::string, std::string>& labels)
    : name_(name), help_(help), labels_(labels) {}

void Counter::increment(double value) {
    double current = value_.load();
    while (!value_.compare_exchange_weak(current, current + value)) {
        // Retry on failure
    }
}

double Counter::getValue() const {
    return value_.load();
}

std::string Counter::render() const {
    std::ostringstream oss;
    
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " counter\n";
    oss << name_;
    
    if (!labels_.empty()) {
        oss << "{";
        bool first = true;
        for (const auto& [key, val] : labels_) {
            if (!first) oss << ",";
            oss << key << "=\"" << val << "\"";
            first = false;
        }
        oss << "}";
    }
    
    oss << " " << std::fixed << std::setprecision(2) << value_.load() << "\n";
    return oss.str();
}

// ==================== Gauge Implementation ====================

Gauge::Gauge(const std::string& name, const std::string& help,
             const std::map<std::string, std::string>& labels)
    : name_(name), help_(help), labels_(labels) {}

void Gauge::set(double value) {
    value_.store(value);
}

void Gauge::increment(double value) {
    double current = value_.load();
    while (!value_.compare_exchange_weak(current, current + value)) {
        // Retry on failure
    }
}

void Gauge::decrement(double value) {
    increment(-value);
}

double Gauge::getValue() const {
    return value_.load();
}

std::string Gauge::render() const {
    std::ostringstream oss;
    
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " gauge\n";
    oss << name_;
    
    if (!labels_.empty()) {
        oss << "{";
        bool first = true;
        for (const auto& [key, val] : labels_) {
            if (!first) oss << ",";
            oss << key << "=\"" << val << "\"";
            first = false;
        }
        oss << "}";
    }
    
    oss << " " << std::fixed << std::setprecision(2) << value_.load() << "\n";
    return oss.str();
}

// ==================== Histogram Implementation ====================

Histogram::Histogram(const std::string& name, const std::string& help,
                     const std::vector<double>& buckets,
                     const std::map<std::string, std::string>& labels)
    : name_(name), help_(help), labels_(labels) {
    
    if (buckets.empty()) {
        buckets_ = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
    } else {
        buckets_ = buckets;
    }
    
    // Initialize bucket counts  
    bucketCounts_.resize(buckets_.size() + 1, 0);
}

void Histogram::observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update sum and count
    sum_ += value;
    count_++;
    
    // Update bucket counts
    for (size_t i = 0; i < buckets_.size(); i++) {
        if (value <= buckets_[i]) {
            bucketCounts_[i]++;
        }
    }
    // +Inf bucket
    bucketCounts_[buckets_.size()]++;
}

std::string Histogram::render() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " histogram\n";
    
    // Render buckets
    for (size_t i = 0; i < buckets_.size(); i++) {
        oss << name_ << "_bucket{";
        
        bool first = true;
        for (const auto& [key, val] : labels_) {
            if (!first) oss << ",";
            oss << key << "=\"" << val << "\"";
            first = false;
        }
        
        if (!first) oss << ",";
        oss << "le=\"" << buckets_[i] << "\"} " 
            << bucketCounts_[i] << "\n";
    }
    
    // +Inf bucket
    oss << name_ << "_bucket{";
    bool first = true;
    for (const auto& [key, val] : labels_) {
        if (!first) oss << ",";
        oss << key << "=\"" << val << "\"";
        first = false;
    }
    if (!first) oss << ",";
    oss << "le=\"+Inf\"} " << bucketCounts_[buckets_.size()] << "\n";
    
    // Sum
    oss << name_ << "_sum";
    if (!labels_.empty()) {
        oss << "{";
        first = true;
        for (const auto& [key, val] : labels_) {
            if (!first) oss << ",";
            oss << key << "=\"" << val << "\"";
            first = false;
        }
        oss << "}";
    }
    oss << " " << std::fixed << std::setprecision(6) << sum_ << "\n";
    
    // Count
    oss << name_ << "_count";
    if (!labels_.empty()) {
        oss << "{";
        first = true;
        for (const auto& [key, val] : labels_) {
            if (!first) oss << ",";
            oss << key << "=\"" << val << "\"";
            first = false;
        }
        oss << "}";
    }
    oss << " " << count_ << "\n";
    
    return oss.str();
}

// ==================== PrometheusExporter Implementation ====================

PrometheusExporter::PrometheusExporter() {}

PrometheusExporter::~PrometheusExporter() {}

std::shared_ptr<Counter> PrometheusExporter::registerCounter(
    const std::string& name,
    const std::string& help,
    const std::map<std::string, std::string>& labels) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto counter = std::make_shared<Counter>(name, help, labels);
    metrics_[name] = counter;
    return counter;
}

std::shared_ptr<Gauge> PrometheusExporter::registerGauge(
    const std::string& name,
    const std::string& help,
    const std::map<std::string, std::string>& labels) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto gauge = std::make_shared<Gauge>(name, help, labels);
    metrics_[name] = gauge;
    return gauge;
}

std::shared_ptr<Histogram> PrometheusExporter::registerHistogram(
    const std::string& name,
    const std::string& help,
    const std::vector<double>& buckets,
    const std::map<std::string, std::string>& labels) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto histogram = std::make_shared<Histogram>(name, help, buckets, labels);
    metrics_[name] = histogram;
    return histogram;
}

std::string PrometheusExporter::renderMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream oss;
    for (const auto& [name, metric] : metrics_) {
        oss << metric->render();
    }
    return oss.str();
}

PrometheusExporter& PrometheusExporter::getInstance() {
    static PrometheusExporter instance;
    return instance;
}

// ==================== AILEEMetrics Implementation ====================

AILEEMetrics::AILEEMetrics() {
    initializeMetrics();
}

void AILEEMetrics::initializeMetrics() {
    auto& exporter = PrometheusExporter::getInstance();
    
    // Node metrics
    nodeCount = exporter.registerGauge("ailee_nodes_total", "Total number of AILEE nodes");
    activePeers = exporter.registerGauge("ailee_peers_active", "Number of active peer connections");
    
    // Task metrics
    tasksTotal = exporter.registerCounter("ailee_tasks_total", "Total number of tasks created");
    tasksCompleted = exporter.registerCounter("ailee_tasks_completed", "Number of completed tasks");
    tasksFailed = exporter.registerCounter("ailee_tasks_failed", "Number of failed tasks");
    tasksPending = exporter.registerGauge("ailee_tasks_pending", "Number of pending tasks");
    tasksRunning = exporter.registerGauge("ailee_tasks_running", "Number of running tasks");
    taskDuration = exporter.registerHistogram("ailee_task_duration_seconds", 
                                               "Task execution duration in seconds",
                                               {0.01, 0.05, 0.1, 0.5, 1.0, 5.0, 10.0, 30.0, 60.0});
    
    // Network metrics
    networkBytesSent = exporter.registerCounter("ailee_network_bytes_sent", "Total bytes sent over network");
    networkBytesReceived = exporter.registerCounter("ailee_network_bytes_received", "Total bytes received from network");
    networkMessagesSent = exporter.registerCounter("ailee_network_messages_sent", "Total messages sent");
    networkMessagesReceived = exporter.registerCounter("ailee_network_messages_received", "Total messages received");
    
    // Storage metrics
    storageOperations = exporter.registerCounter("ailee_storage_operations_total", "Total storage operations");
    storageLatency = exporter.registerHistogram("ailee_storage_latency_seconds", 
                                                  "Storage operation latency",
                                                  {0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0});
    storageSizeBytes = exporter.registerGauge("ailee_storage_size_bytes", "Total storage size in bytes");
    
    // Bitcoin metrics
    bitcoinBlockHeight = exporter.registerGauge("ailee_bitcoin_block_height", "Current Bitcoin block height");
    bitcoinTransactions = exporter.registerCounter("ailee_bitcoin_transactions_total", "Total Bitcoin transactions processed");
    
    // System metrics
    uptimeSeconds = exporter.registerGauge("ailee_uptime_seconds", "Node uptime in seconds");
    memoryUsageBytes = exporter.registerGauge("ailee_memory_usage_bytes", "Memory usage in bytes");
    cpuUsagePercent = exporter.registerGauge("ailee_cpu_usage_percent", "CPU usage percentage");
}

AILEEMetrics& AILEEMetrics::getInstance() {
    static AILEEMetrics instance;
    return instance;
}

} // namespace ailee::metrics
