#pragma once

#include "l6/IslaRuntimeOrchestrator.h"
#include <string>

// Forward declare rocksdb types to avoid including heavy headers
namespace rocksdb {
    class DB;
}

namespace ailee::l6 {

struct ReplayBufferConfig {
    std::string db_path;
};

class ReplayBuffer : public IReplayBuffer {
public:
    explicit ReplayBuffer(const ReplayBufferConfig& config);
    ~ReplayBuffer() override;

    void record_epoch(const EpochIntegrationBundle& bundle, const IslaEpochResult& result) override;

    std::vector<EpochIntegrationBundle> get_epoch_history() const override;
    void remove_oldest() override;
    size_t size() const override;
    size_t max_size() const override;

private:
    rocksdb::DB* db_;
};

} // namespace ailee::l6
