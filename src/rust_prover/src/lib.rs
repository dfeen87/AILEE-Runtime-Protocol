// FFI boundary for Halo2 proof generation and verification

use std::ffi::{c_char, CStr, CString};
use std::os::raw::c_int;

#[no_mangle]
pub extern "C" fn generate_halo2_proof_ffi(
    task_id: *const c_char,
    computation_hash: *const c_char,
    out_proof: *mut *mut c_char,
) -> c_int {
    if task_id.is_null() || computation_hash.is_null() || out_proof.is_null() {
        return -1;
    }

    let _task_id_str = unsafe { CStr::from_ptr(task_id).to_string_lossy() };
    let computation_hash_str = unsafe { CStr::from_ptr(computation_hash).to_string_lossy() };

    // Placeholder: In a real system, we would construct a circuit and run the Halo2 prover here.
    // For now, generate a mock proof string incorporating the computation hash.
    let proof_str = format!("halo2_proof_mock_{}", computation_hash_str);

    let c_str_proof = match CString::new(proof_str) {
        Ok(s) => s,
        Err(_) => return -1,
    };

    unsafe {
        *out_proof = c_str_proof.into_raw();
    }

    0
}

#[no_mangle]
pub extern "C" fn verify_halo2_proof_ffi(
    proof_data: *const c_char,
    computation_hash: *const c_char,
) -> c_int {
    if proof_data.is_null() || computation_hash.is_null() {
        return -1;
    }

    let proof_str = unsafe { CStr::from_ptr(proof_data).to_string_lossy() };
    let computation_hash_str = unsafe { CStr::from_ptr(computation_hash).to_string_lossy() };

    // Placeholder: Real system parses proof_data and verifies against the public input (computation_hash)
    // using Halo2 verifier.
    let expected_mock = format!("halo2_proof_mock_{}", computation_hash_str);
    if proof_str == expected_mock {
        1 // True
    } else {
        0 // False
    }
}

#[no_mangle]
pub extern "C" fn free_halo2_proof_ffi(proof_ptr: *mut c_char) {
    if !proof_ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(proof_ptr);
        }
    }
}
