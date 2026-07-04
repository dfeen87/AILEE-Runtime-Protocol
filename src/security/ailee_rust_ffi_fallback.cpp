// C++ Fallback for Rust FFI boundary
// This allows compiling the codebase without a Rust toolchain during the transition.
#include "ailee_rust_ffi.h"
#include <cstring>
#include <string>
#include <cstdlib>

extern "C" {

#ifndef AILEE_USE_RUST_PROVER

int generate_halo2_proof_ffi(const char* task_id, const char* computation_hash, char** out_proof) {
    if (!task_id || !computation_hash || !out_proof) {
        return -1;
    }
    std::string proof_str = "halo2_proof_mock_" + std::string(computation_hash);
    *out_proof = strdup(proof_str.c_str());
    return 0;
}

int verify_halo2_proof_ffi(const char* proof_data, const char* computation_hash) {
    if (!proof_data || !computation_hash) {
        return -1;
    }
    std::string expected = "halo2_proof_mock_" + std::string(computation_hash);
    if (expected == proof_data) {
        return 1;
    }
    return 0;
}

void free_halo2_proof_ffi(char* proof_ptr) {
    if (proof_ptr) {
        free(proof_ptr);
    }
}

#endif // AILEE_USE_RUST_PROVER

}
