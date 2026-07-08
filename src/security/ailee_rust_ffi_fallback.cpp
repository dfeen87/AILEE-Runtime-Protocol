// C++ Fallback for Rust FFI boundary
// This allows compiling the codebase without a Rust toolchain during the transition.
#include "ailee_rust_ffi.h"
#include <cstring>
#include <string>
#include <cstdlib>

extern "C" {

#ifndef AILEE_USE_RUST_PROVER

int generate_halo2_proof_ffi(const char* task_id, const char* computation_hash, Halo2ProofOutput** out_proof) {
    if (!task_id || !computation_hash || !out_proof) {
        return -1;
    }
    std::string proof_str = "halo2_proof_mock_" + std::string(computation_hash);

    Halo2ProofOutput* output = new Halo2ProofOutput();

    unsigned char* proof_bytes = new unsigned char[proof_str.length()];
    std::memcpy(proof_bytes, proof_str.c_str(), proof_str.length());
    output->proof_ptr = proof_bytes;
    output->proof_len = proof_str.length();

    // Mock 96 byte commitment
    unsigned char* commitment_bytes = new unsigned char[96];
    std::memset(commitment_bytes, 0, 96);
    output->commitment_ptr = commitment_bytes;
    output->commitment_len = 96;

    *out_proof = output;
    return 0;
}

int verify_halo2_proof_ffi(const unsigned char* proof_data, size_t proof_len, const char* computation_hash) {
    if (!proof_data || !computation_hash) {
        return -1;
    }
    std::string expected = "halo2_proof_mock_" + std::string(computation_hash);
    std::string provided(reinterpret_cast<const char*>(proof_data), proof_len);
    if (expected == provided) {
        return 1;
    }
    return 0;
}

void free_halo2_proof_ffi(Halo2ProofOutput* proof_ptr) {
    if (proof_ptr) {
        if (proof_ptr->proof_ptr) {
            delete[] proof_ptr->proof_ptr;
        }
        if (proof_ptr->commitment_ptr) {
            delete[] proof_ptr->commitment_ptr;
        }
        delete proof_ptr;
    }
}

int init_network_ffi() {
    return -1;
}

int broadcast_message_ffi(const char* topic, const unsigned char* payload, size_t payload_len) {
    if (!topic || !payload || payload_len == 0) {
        return -1;
    }
    return -1;
}

int subscribe_topic_ffi(const char* topic) {
    if (!topic) {
        return -1;
    }
    return -1;
}

#endif // AILEE_USE_RUST_PROVER

}
