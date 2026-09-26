#include "ailee/ledger/AlcoaLedger.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ailee::ledger {

std::string AlcoaLedger::record_entry(AlcoaEntry entry) {
    if (entry.timestamp_utc == 0) {
        entry.timestamp_utc = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }

    if (entry.entry_id.empty()) {
        std::stringstream ss;
        ss << "alcoa-" << ledger_entries_.size() + 1 << "-" << entry.timestamp_utc;
        entry.entry_id = ss.str();
    }

    if (!ledger_entries_.empty() && entry.parent_entry_id.empty()) {
        entry.parent_entry_id = ledger_entries_.back().entry_id;
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
            bool accurate = !entry.posture_regime_id.empty();

            return attributable && legible && contemporaneous && original && accurate;
        }
    }
    return false;
}

AlcoaEntry AlcoaLedger::get_latest_entry() const {
    if (ledger_entries_.empty()) {
        return AlcoaEntry{};
    }
    return ledger_entries_.back();
}

} // namespace ailee::ledger
