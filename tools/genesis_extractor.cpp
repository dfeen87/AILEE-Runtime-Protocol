#include "../include/ambient_ai_node_identity.hpp"
#include "../include/ambient_ai_l2_merkleization.hpp"
#include "../include/ambient_ai_epoch.hpp"
#include "../src/wnn/core/wave_native_network_core.hpp"
#include "../include/ambient_ai_consensus_state_machine.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>
#include <cstring>

std::string to_hex(const std::array<uint8_t, 32>& hash) {
    std::stringstream ss;
    ss << "0x";
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int main() {
    // 1. Genesis State Root
    // The core root from AmbientL2Merkleizer using empty parameters
    ailee::consensus::AmbientL2Merkleizer merkleizer;
    std::vector<ailee::consensus::Hash256> empty_leaves;
    ailee::consensus::Hash256 genesisStateRoot = merkleizer.computeMerkleRoot(empty_leaves);

    // 2. Wave-State Commitment
    // A default WaveState object initialized with zeros, properly serialized field-by-field, and hashed
    ailee::wnn::WaveState waveState = {0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<uint8_t> waveStateBytes;
    auto push_double = [&waveStateBytes](double val) {
        uint64_t bits;
        std::memcpy(&bits, &val, sizeof(bits));
        for (int i = 7; i >= 0; --i) {
            waveStateBytes.push_back((bits >> (i * 8)) & 0xFF);
        }
    };
    push_double(waveState.A);
    push_double(waveState.theta);
    push_double(waveState.omega);
    push_double(waveState.x_dot);
    push_double(waveState.ts);

    ailee::consensus::Hash256 waveStateCommitment;
    SHA256(waveStateBytes.data(), waveStateBytes.size(), waveStateCommitment.data());

    // 3. Identity Baseline
    // Default-initialized IdentityPayload, hashed via canonical Hash function
    ailee::identity::IdentityPayload identity;
    identity.peerId = "";
    identity.publicKeyHex = "";
    identity.metadata = {"", 0, 0};
    identity.epochId = 0;
    identity.staticAttribute = 0;
    ailee::identity::Hash256 identityBaseline = identity.hash();

    // 4. Participation Baseline
    // Properly serialize the empty ParticipationProof to hash it
    ailee::identity::ParticipationProof proof;
    proof.payload = identity;
    // signature is empty
    std::vector<uint8_t> proofBytes = identity.serialize(); // use the canonical serialize()
    // Append length of signature
    uint32_t sigLen = proof.ecdsaSignature.size();
    for (int i = 3; i >= 0; --i) {
        proofBytes.push_back((sigLen >> (i * 8)) & 0xFF);
    }
    proofBytes.insert(proofBytes.end(), proof.ecdsaSignature.begin(), proof.ecdsaSignature.end());

    ailee::identity::Hash256 participationBaseline;
    SHA256(proofBytes.data(), proofBytes.size(), participationBaseline.data());

    // Print all to stdout
    std::cout << "# V12 Genesis Freeze\n\n";
    std::cout << "## Genesis State Root\n";
    std::cout << "`" << to_hex(genesisStateRoot) << "`\n\n";

    std::cout << "## Wave-State Commitment\n";
    std::cout << "`" << to_hex(waveStateCommitment) << "`\n\n";

    std::cout << "## Identity Baseline\n";
    std::cout << "`" << to_hex(identityBaseline) << "`\n\n";

    std::cout << "## Participation Baseline\n";
    std::cout << "`" << to_hex(participationBaseline) << "`\n\n";

    std::cout << "## Extraction Method\n";
    std::cout << "- Instantiated AILEE CORE subsystems with default genesis parameters.\n";
    std::cout << "- Computed canonical commitments via subsystem hash functions.\n";
    std::cout << "- All values are deterministic and reproducible.\n";

    return 0;
}
