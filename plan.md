1. **Define Core Semantics Structs in `ailee::semantics`:**
   - Define `EnvironmentType` enum (`CI`, `DEV`, `PROD`).
   - Modify `l6::RuntimeEnvironment` to support this enum alongside `is_ci`. Map `is_ci == true` to `EnvironmentType::CI`, otherwise to `DEV` by default.
   - Define `Config` struct. Includes `anchor_frequency`, `proof_frequency`, `max_replay_epochs`, `min_coherence_score`, `epoch_length` (default 1000), `environment_type`, `metadata_version`.
   - Define `PolicyState` struct to mirror rules. `PolicyState load_policy(const Config&);`
   - Define `MeshState` struct (`node_count`, `active_links`, `latency_mean`, `latency_variance`, `partition_count`, `recent_failures`).
   - Define `CanonicalMetadata` struct (`protocol_version`, `epoch_id`, `state_root_hash`, `backend_type`, `proof_attached`, `coherence_score`).

2. **Implement Epoch Semantics (`ailee::semantics::EpochSemantics`):**
   - Implement `compute_epoch_id(const ClockSnapshot& snapshot)` using `floor(clock_height / BLOCKS_PER_EPOCH)`.
   - Implement `validate_epoch_bundle(const EpochIntegrationBundle& bundle, const l6::IReplayBuffer& replay_buffer)`.
     - Validates `state_root_hash` equals hex-encoded SHA-256 of `transcript->to_bytes()`.
     - Checks `constraints` and `transcript` are non-null and not empty.
     - Checks `clock_height >= previous_height` where `previous_height` comes from `replay_buffer`.
     - Validates `epoch_id == compute_epoch_id(clock_height)`.

3. **Implement Clock Semantics (`ailee::semantics::ClockSemantics`):**
   - Expand `l6::ClockSnapshot` with `std::string hash` (64 hex chars) and `std::string source` (must be "Bitcoin" in PROD).
   - Implement `is_valid_snapshot(const l6::ClockSnapshot& snapshot, const l6::IReplayBuffer& replay_buffer)`.
   - Implement `is_epoch_boundary(const l6::ClockSnapshot& snapshot, const l6::ClockSnapshot& previous)`.

4. **Implement Mesh Coherence Semantics:**
   - Implement `compute_coherence_score(const MeshState& state)` handling `node_count == 0` safely.
   - Implement `is_coherent_enough(double score, const PolicyState& policy)`.

5. **Implement Anchor and Proof Attachment Semantics:**
   - Implement `should_anchor(const l6::SchedulerDecision&, const PolicyState&, const MeshState&)` or similar.
   - Implement `should_attach_proof(const l6::SchedulerDecision&, const PolicyState&, l6::ZKBackendType)`.

6. **Implement Replay Semantics:**
   - Expand `l6::IReplayBuffer` with `std::vector<EpochIntegrationBundle> get_epoch_history() const`, `void remove_oldest()`, `size_t size() const`, and `size_t max_size() const`.
   - Add these methods to `l6::ReplayBuffer` and `tests/l6/test_isla_determinism.cpp` stub.
   - Implement `validate_replay_chain(const l6::IReplayBuffer& buffer)`.
   - Implement `enforce_replay_retention(l6::IReplayBuffer& buffer, size_t max_epochs)`.

7. **Implement Backend Activation Semantics:**
   - Implement `is_backend_allowed(EnvironmentType env, l6::ZKBackendType backend_type)`. CI -> MOCK. DEV -> MOCK/HALO2. PROD -> HALO2/PLONK.

8. **Implement Metadata Semantics:**
   - Implement `encode_metadata(const l6::EpochIntegrationBundle&, const l6::IslaEpochResult&)`.
   - Implement `hash_canonical_metadata(const CanonicalMetadata&)`.

9. **Integrate Semantics into `IslaRuntimeOrchestrator`:**
   - Update `attach_backend` to use `is_backend_allowed` from semantics.
   - In `run_epoch`:
     - Load `PolicyState` and `MeshState`.
     - Extract `previous_height` from `IReplayBuffer`.
     - Call `validate_epoch_bundle`.
     - Use `should_anchor` and `should_attach_proof` to override plans instead of implicit logic.
     - Replace `AnchorMetadataEncoder` logic with `CanonicalMetadata` logic.
     - Call `enforce_replay_retention`.

10. **Build and Run Tests:**
    - Add comprehensive unit tests in `tests/test_isla_semantics.cpp` for the semantics components.
    - Run the test suite: `./build/ailee_tests`.

11. **Pre-commit step:**
    - Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

12. **Submit**
