#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ailee::ledger {

struct AlcoaEntry {
    // Attributable
    std::string operator_id;
    std::string system_id;
    std::string operator_signature;

    // Legible
    std::string human_readable_summary;
    std::string regime_label;
    std::string compartment_label;

    // Contemporaneous
    uint64_t timestamp_utc{0};
    uint64_t epoch_id{0};
    std::string epoch_hash;

    // Original
    std::string entry_id;
    std::string parent_entry_id;
    std::string source_system;

    // Accurate
    std::string posture_regime_id;
    double posture_score{0.0};
    std::string zk_recursion_root;
    double temporal_coherence_index{0.0};
    double signal_energy{0.0};
    double coherence_score{0.0};
};

class AlcoaLedger {
public:
    AlcoaLedger() = default;

    std::string record_entry(AlcoaEntry entry);
    bool verify_entry(const std::string& entry_id) const;
    const std::vector<AlcoaEntry>& entries() const { return ledger_entries_; }
    AlcoaEntry get_latest_entry() const;
    size_t size() const { return ledger_entries_.size(); }

private:
    std::vector<AlcoaEntry> ledger_entries_;
};

} // namespace ailee::ledger
