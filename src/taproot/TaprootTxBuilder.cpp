#include "taproot/TaprootTxBuilder.h"

namespace ailee {
namespace taproot {

namespace {

// Helper to write a 32-bit integer in little-endian
void write_uint32_le(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

// Helper to write a 64-bit integer in little-endian
void write_uint64_le(std::vector<uint8_t>& out, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((val >> (8 * i)) & 0xFF));
    }
}

// Helper to write a Bitcoin CompactSize (varint)
void write_compact_size(std::vector<uint8_t>& out, uint64_t val) {
    if (val < 0xFD) {
        out.push_back(static_cast<uint8_t>(val));
    } else if (val <= 0xFFFF) {
        out.push_back(0xFD);
        out.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    } else if (val <= 0xFFFFFFFF) {
        out.push_back(0xFE);
        out.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    } else {
        out.push_back(0xFF);
        write_uint64_le(out, val);
    }
}

} // anonymous namespace

std::vector<uint8_t> TxOut::to_bytes() const {
    std::vector<uint8_t> out;
    write_uint64_le(out, value_sats);
    write_compact_size(out, script_pubkey.size());
    out.insert(out.end(), script_pubkey.begin(), script_pubkey.end());
    return out;
}

std::vector<uint8_t> TxIn::to_bytes() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), prev_txid.begin(), prev_txid.end()); // prev_txid is already little-endian
    write_uint32_le(out, prev_vout);
    write_compact_size(out, script_sig.size());
    out.insert(out.end(), script_sig.begin(), script_sig.end());
    write_uint32_le(out, sequence);
    return out;
}

std::vector<uint8_t> Tx::to_bytes() const {
    std::vector<uint8_t> out;
    write_uint32_le(out, version);

    write_compact_size(out, vin.size());
    for (const auto& in : vin) {
        std::vector<uint8_t> in_bytes = in.to_bytes();
        out.insert(out.end(), in_bytes.begin(), in_bytes.end());
    }

    write_compact_size(out, vout.size());
    for (const auto& o : vout) {
        std::vector<uint8_t> o_bytes = o.to_bytes();
        out.insert(out.end(), o_bytes.begin(), o_bytes.end());
    }

    write_uint32_le(out, locktime);
    return out;
}

std::vector<uint8_t> build_p2tr_script_pubkey(const std::array<uint8_t, 32>& taproot_output_key) {
    std::vector<uint8_t> script;
    script.reserve(34); // 1 + 1 + 32
    script.push_back(0x51); // OP_1
    script.push_back(0x20); // OP_PUSH32
    script.insert(script.end(), taproot_output_key.begin(), taproot_output_key.end());
    return script;
}

Tx build_taproot_tx(const std::array<uint8_t, 32>& prev_txid,
                    uint32_t prev_vout,
                    uint64_t /*input_value_sats*/,
                    const TaprootOutput& taproot_output,
                    uint64_t output_value_sats,
                    uint32_t locktime) {
    TxIn txin;
    txin.prev_txid = prev_txid;
    txin.prev_vout = prev_vout;
    txin.script_sig.clear(); // empty for unsigned
    txin.sequence = 0xFFFFFFFF;

    TxOut txout;
    txout.value_sats = output_value_sats;
    txout.script_pubkey = build_p2tr_script_pubkey(taproot_output.taproot_output_key);

    Tx tx;
    tx.version = 2;
    tx.vin.push_back(txin);
    tx.vout.push_back(txout);
    tx.locktime = locktime;

    return tx;
}

} // namespace taproot
} // namespace ailee
