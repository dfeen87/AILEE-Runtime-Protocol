#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// FFI declarations for Halo2 Rust prover
int generate_halo2_proof_ffi(const char* task_id, const char* computation_hash, char** out_proof);
int verify_halo2_proof_ffi(const char* proof_data, const char* computation_hash);
void free_halo2_proof_ffi(char* proof_ptr);

// Network FFI functions for libp2p via rust-libp2p
int init_network_ffi();
int broadcast_message_ffi(const char* topic, const unsigned char* payload, unsigned long payload_len);
int subscribe_topic_ffi(const char* topic);

#ifdef __cplusplus
}
#endif
