#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// FFI declarations for Halo2 Rust prover
int generate_halo2_proof_ffi(const char* task_id, const char* computation_hash, char** out_proof);
int verify_halo2_proof_ffi(const char* proof_data, const char* computation_hash);
void free_halo2_proof_ffi(char* proof_ptr);

#ifdef __cplusplus
}
#endif
