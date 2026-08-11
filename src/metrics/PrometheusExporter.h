// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// PrometheusExporter.h — Prometheus metrics export for AILEE-Core

#pragma once

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <atomic>
#include <vector>

namespace ailee::metrics {

/**
 * Metric types supported by Prometheus
 */
enum class MetricType {
    COUNTER,     // Monotonically increasing counter
    GAUGE,       // Value that can go up or down
    HISTOGRAM,   // Distribution of values
    SUMMARY      // Summary statistics
};

/**
 * Prometheus metric interface
 */
class Metric {
public:
    virtual ~Metric() = default;
    virtual MetricType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getHelp() const = 0;
    virtual std::string render() const = 0;
};

/**
 * Counter metric (monotonically increasing)
 */
class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& help, 
            const std::map<std::string, std::string>& labels = {});
    
    void increment(double value = 1.0);
    double getValue() const;
    
    MetricType getType() const override { return MetricType::COUNTER; }
    std::string getName() const override { return name_; }
    std::string getHelp() const override { return help_; }
    std::string render() const override;

private:
    std::string name_;
    std::string help_;
    std::map<std::string, std::string> labels_;
    std::atomic<double> value_{0.0};
};

/**
 * Gauge metric (can go up or down)
 */
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& help,
          const std::map<std::string, std::string>& labels = {});
    
    void set(double value);
    void increment(double value = 1.0);
    void decrement(double value = 1.0);
    double getValue() const;
    
    MetricType getType() const override { return MetricType::GAUGE; }
    std::string getName() const override { return name_; }
    std::string getHelp() const override { return help_; }
    std::string render() const override;

private:
    std::string name_;
    std::string help_;
    std::map<std::string, std::string> labels_;
    std::atomic<double> value_{0.0};
};

/**
 * Histogram metric (distribution of values)
 */
class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& help,
              const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0},
              const std::map<std::string, std::string>& labels = {});
    
    void observe(double value);
    
    MetricType getType() const override { return MetricType::HISTOGRAM; }
    std::string getName() const override { return name_; }
    std::string getHelp() const override { return help_; }
    std::string render() const override;

private:
    std::string name_;
    std::string help_;
    std::vector<double> buckets_;
    std::map<std::string, std::string> labels_;
    std::vector<uint64_t> bucketCounts_;  // Use regular vector, protect with mutex
    double sum_{0.0};
    uint64_t count_{0};
    mutable std::mutex mutex_;
};

/**
 * Prometheus Exporter
 * 
 * Collects and exposes metrics in Prometheus text format
 */
class PrometheusExporter {
public:
    PrometheusExporter();
    ~PrometheusExporter();
    
    /**
     * Register a counter metric
     */
    std::shared_ptr<Counter> registerCounter(
        const std::string& name,
        const std::string& help,
        const std::map<std::string, std::string>& labels = {}
    );
    
    /**
     * Register a gauge metric
     */
    std::shared_ptr<Gauge> registerGauge(
        const std::string& name,
        const std::string& help,
        const std::map<std::string, std::string>& labels = {}
    );
    
    /**
     * Register a histogram metric
     */
    std::shared_ptr<Histogram> registerHistogram(
        const std::string& name,
        const std::string& help,
        const std::vector<double>& buckets = {},
        const std::map<std::string, std::string>& labels = {}
    );
    
    /**
     * Render all metrics in Prometheus text format
     */
    std::string renderMetrics() const;
    
    /**
     * Get singleton instance
     */
    static PrometheusExporter& getInstance();

private:
    std::map<std::string, std::shared_ptr<Metric>> metrics_;
    mutable std::mutex mutex_;
};

/**
 * AILEE-Core specific metrics
 */
class AILEEMetrics {
public:
    static AILEEMetrics& getInstance();
    
    // Node metrics
    std::shared_ptr<Gauge> nodeCount;
    std::shared_ptr<Gauge> activePeers;
    
    // Task metrics
    std::shared_ptr<Counter> tasksTotal;
    std::shared_ptr<Counter> tasksCompleted;
    std::shared_ptr<Counter> tasksFailed;
    std::shared_ptr<Gauge> tasksPending;
    std::shared_ptr<Gauge> tasksRunning;
    std::shared_ptr<Histogram> taskDuration;
    
    // Network metrics
    std::shared_ptr<Counter> networkBytesSent;
    std::shared_ptr<Counter> networkBytesReceived;
    std::shared_ptr<Counter> networkMessagesSent;
    std::shared_ptr<Counter> networkMessagesReceived;
    
    // Storage metrics
    std::shared_ptr<Counter> storageOperations;
    std::shared_ptr<Histogram> storageLatency;
    std::shared_ptr<Gauge> storageSizeBytes;
    
    // Bitcoin metrics
    std::shared_ptr<Gauge> bitcoinBlockHeight;
    std::shared_ptr<Counter> bitcoinTransactions;
    
    // System metrics
    std::shared_ptr<Gauge> uptimeSeconds;
    std::shared_ptr<Gauge> memoryUsageBytes;
    std::shared_ptr<Gauge> cpuUsagePercent;

private:
    AILEEMetrics();
    void initializeMetrics();
};

} // namespace ailee::metrics
