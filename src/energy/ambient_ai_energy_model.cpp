#include <openssl/sha.h>
#include "ambient_ai_energy_model.hpp"
#include <string.h>

namespace ailee {
namespace energy {

std::vector<uint8_t> EnergyProfile::canonicalSerialize() const {
    // Length-prefixed binary form for Merkle leaf creation
    // pubkey len (4) + pubkey + 3 * uint64_t (24)
    std::vector<uint8_t> out;
    uint32_t pkLen = static_cast<uint32_t>(publicKeyHex.size());
    out.reserve(4 + pkLen + 24);

    out.push_back(static_cast<uint8_t>((pkLen >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((pkLen >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((pkLen >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(pkLen & 0xFF));

    out.insert(out.end(), publicKeyHex.begin(), publicKeyHex.end());

    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
        }
    };

    push64(computeCompletedJobs);
    push64(uptimeMilliseconds);
    push64(storageBytesProvided);

    return out;
}

Hash256 EnergyProfile::hash() const {
    auto bytes = canonicalSerialize();
    // Using libsecp256k1 wrapper for SHA256 (which we simulate via openssl for now as the architecture asks for secp256k1 equivalence)
    // The requirement says: "Must strictly use libsecp256k1 equivalent". We will use libsecp256k1 directly if available, or fallback.
    // We already have openssl sha256. We'll use secp256k1 if included, but standard OpenSSL SHA256 is byte-equivalent.
    // Actually, let's include secp256k1.h and see if we can use it. Wait, secp256k1 doesn't expose SHA256 in its public API by default (it's internal).
    // The architecture says "exact SHA-256 byte-level implementations matching libsecp256k1". We will use openssl SHA256 which is identical.

    Hash256 out;

    SHA256(bytes.data(), bytes.size(), out.data());
    return out;
}

}
}
