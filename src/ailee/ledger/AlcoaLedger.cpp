#include "ailee/ledger/AlcoaLedger.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ailee::ledger {

std::string AlcoaLedger::compute_entry_hash(const AlcoaEntry& entry) {
    std::stringstream ss;
    ss << entry.parent_entry_id << "|"
       << entry.operator_id << "|"
       << entry.system_id << "|"
       << entry.operator_signature << "|"
       << entry.human_readable_summary << "|"
       << entry.regime_label << "|"
       << entry.compartment_label << "|"
       << entry.timestamp_utc << "|"
       << entry.epoch_id << "|"
       << entry.epoch_hash << "|"
       << entry.source_system << "|"
       << entry.posture_regime_id << "|"
       << entry.posture_score << "|"
       << entry.zk_recursion_root << "|"
       << entry.temporal_coherence_index << "|"
       << entry.signal_energy << "|"
       << entry.coherence_score;

    std::string raw = ss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(raw.data()), raw.size(), hash);

    std::stringstream hex_ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return "alcoa-0x" + hex_ss.str().substr(0, 32);
}

std::string AlcoaLedger::record_entry(AlcoaEntry entry) {
    if (entry.timestamp_utc == 0) {
        entry.timestamp_utc = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }

    if (ledger_entries_.empty()) {
        if (entry.parent_entry_id.empty()) {
            entry.parent_entry_id = "0x0000000000000000000000000000000000000000000000000000000000000000";
        }
    } else {
        if (entry.parent_entry_id.empty()) {
            entry.parent_entry_id = ledger_entries_.back().entry_id;
        }
    }

    if (entry.entry_id.empty()) {
        entry.entry_id = compute_entry_hash(entry);
    }

    ledger_entries_.push_back(entry);
    return entry.entry_id;
}

bool AlcoaLedger::verify_entry(const std::string& entry_id) const {
    for (const auto& entry : ledger_entries_) {
        if (entry.entry_id == entry_id) {
            // Check mandatory ALCOA properties
            bool attributable = !entry.operator_id.empty() && !entry.operator_signature.empty();
            bool legible = !entry.human_readable_summary.empty() && !entry.regime_label.empty();
            bool contemporaneous = entry.timestamp_utc > 0 && !entry.epoch_hash.empty();
            bool original = !entry.entry_id.empty();
            bool accurate = !entry.posture_regime_id.empty() &&
                            entry.temporal_coherence_index >= 0.0 && entry.temporal_coherence_index <= 1.0;

            return attributable && legible && contemporaneous && original && accurate;
        }
    }
    return false;
}

bool AlcoaLedger::verify_chain() const {
    if (ledger_entries_.empty()) {
        return true;
    }

    for (size_t i = 0; i < ledger_entries_.size(); ++i) {
        const auto& entry = ledger_entries_[i];
        if (!verify_entry(entry.entry_id)) {
            return false;
        }

        if (i > 0) {
            if (entry.parent_entry_id != ledger_entries_[i - 1].entry_id) {
                return false; // Parent hash link broken
            }
        }
    }
    return true;
}

AlcoaEntry AlcoaLedger::get_latest_entry() const {
    if (ledger_entries_.empty()) {
        return AlcoaEntry{};
    }
    return ledger_entries_.back();
}

} // namespace ailee::ledger
