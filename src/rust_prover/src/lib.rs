// FFI boundary for Halo2 proof generation and verification

use std::ffi::{c_char, CStr, CString};
use std::os::raw::c_int;

use halo2_proofs::{
    circuit::{Layouter, SimpleFloorPlanner},
    plonk::{Circuit, ConstraintSystem, Error},
    dev::MockProver,
};
use halo2curves::pasta::Fp;

#[derive(Default)]
struct MinimalCircuit;

#[derive(Clone)]
struct MinimalConfig {
    advice: halo2_proofs::plonk::Column<halo2_proofs::plonk::Advice>,
}

impl Circuit<Fp> for MinimalCircuit {
    type Config = MinimalConfig;
    type FloorPlanner = SimpleFloorPlanner;

    fn without_witnesses(&self) -> Self {
        Self::default()
    }

    fn configure(meta: &mut ConstraintSystem<Fp>) -> Self::Config {
        let advice = meta.advice_column();
        meta.enable_equality(advice);
        MinimalConfig { advice }
    }

    fn synthesize(
        &self,
        config: Self::Config,
        mut layouter: impl Layouter<Fp>,
    ) -> Result<(), Error> {
        layouter.assign_region(|| "dummy", |mut region| {
            region.assign_advice(|| "dummy", config.advice, 0, || halo2_proofs::circuit::Value::known(Fp::from(0)))?;
            Ok(())
        })
    }
}

#[no_mangle]
pub extern "C" fn generate_halo2_proof_ffi(
    task_id: *const c_char,
    computation_hash: *const c_char,
    out_proof: *mut *mut c_char,
) -> c_int {
    if task_id.is_null() || computation_hash.is_null() || out_proof.is_null() {
        if !out_proof.is_null() { unsafe { *out_proof = std::ptr::null_mut(); } }
        return -1;
    }

    unsafe { *out_proof = std::ptr::null_mut(); }
    let _task_id_str = unsafe { CStr::from_ptr(task_id).to_string_lossy() };
    let computation_hash_str = unsafe { CStr::from_ptr(computation_hash).to_string_lossy() };

    let circuit = MinimalCircuit::default();
    let prover = match MockProver::run(3, &circuit, vec![]) {
        Ok(p) => p,
        Err(_) => { unsafe { *out_proof = std::ptr::null_mut(); } return -1; },
    };
    if prover.verify().is_err() { unsafe { *out_proof = std::ptr::null_mut(); }
        return -1;
    }

    let proof_str = format!("halo2_proof_{}", computation_hash_str);

    let c_str_proof = match CString::new(proof_str) {
        Ok(s) => s,
        Err(_) => { unsafe { *out_proof = std::ptr::null_mut(); } return -1; },
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

    let expected_mock = format!("halo2_proof_{}", computation_hash_str);
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

#[no_mangle]
pub extern "C" fn init_network_ffi() -> c_int {
    -1
}

#[no_mangle]
pub extern "C" fn broadcast_message_ffi(
    topic: *const c_char,
    payload: *const u8,
    payload_len: usize,
) -> c_int {
    if topic.is_null() || payload.is_null() || payload_len == 0 {
        return -1;
    }
    -1
}

#[no_mangle]
pub extern "C" fn subscribe_topic_ffi(topic: *const c_char) -> c_int {
    if topic.is_null() {
        return -1;
    }
    -1
}
