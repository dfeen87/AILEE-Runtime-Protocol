#include "AnchorCommitTxBuilder.h"
#include "taproot/TaprootScript.h"
#include "taproot/TaprootTxBuilder.h"
#include "taproot/TaprootUtils.h"

namespace ailee {
namespace anchor {

AnchorCommitTx AnchorCommitTxBuilder::build_anchor_commit_tx(
    const std::array<uint8_t, 32>& anchor_root,
    const std::array<uint8_t, 32>& metadata_hash,
    const AnchorCommitInput& input,
    uint64_t fee_sats
) {
    // 1. Calculate the leaf hash from anchor_root and metadata_hash
    taproot::AnchorCommitTapLeaf tapleaf = taproot::TaprootScript::build_anchor_commit_tapleaf(
        anchor_root, metadata_hash);

    // 2. Compute the tweaked taproot output key
    taproot::TaprootOutput taproot_output;
    taproot_output.internal_key = input.internal_key;
    taproot_output.compute_output_key(tapleaf.leaf_hash);

    // 3. Prepare the deterministic P2TR scriptPubKey
    std::vector<uint8_t> script_pubkey = taproot::build_p2tr_script_pubkey(taproot_output.taproot_output_key);

    // 4. Calculate output value
    uint64_t output_value_sats = input.value_sats - fee_sats;

    // 5. Serialize the deterministic raw transaction
    AnchorCommitTx result;
    std::vector<uint8_t>& raw_tx = result.raw_tx;

    // version = 2
    taproot::write_uint32_le(raw_tx, 2);

    // marker = 0x00, flag = 0x01
    raw_tx.push_back(0x00);
    raw_tx.push_back(0x01);

    // 1 input
    taproot::write_compact_size(raw_tx, 1);
    raw_tx.insert(raw_tx.end(), input.prev_txid.begin(), input.prev_txid.end()); // Already little-endian
    taproot::write_uint32_le(raw_tx, input.prev_vout);
    taproot::write_compact_size(raw_tx, 0); // Empty scriptSig
    taproot::write_uint32_le(raw_tx, 0xFFFFFFFF); // sequence

    // 1 output
    taproot::write_compact_size(raw_tx, 1);
    taproot::write_uint64_le(raw_tx, output_value_sats);
    taproot::write_compact_size(raw_tx, script_pubkey.size());
    raw_tx.insert(raw_tx.end(), script_pubkey.begin(), script_pubkey.end());

    // Witness section (1 input, 0 witness items since it's unsigned)
    taproot::write_compact_size(raw_tx, 0); // 0 witness items for the first input

    // locktime = 0
    taproot::write_uint32_le(raw_tx, 0);

    return result;
}

} // namespace anchor
} // namespace ailee
