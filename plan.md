1. **Create `TaprootUtils.h` and `TaprootUtils.cpp`:**
   - Define and implement `write_uint32_le`, `write_uint64_le`, and `write_compact_size`.
   - Place them in the `ailee::taproot` namespace.
   - For `write_compact_size`, use the existing helper implementation from `TaprootTxBuilder.cpp`.

2. **Update `TaprootTxBuilder.cpp` and `TaprootSigner.cpp`:**
   - Remove the anonymous namespace definitions of `write_uint32_le`, `write_uint64_le`, and `write_compact_size`.
   - Include `TaprootUtils.h`.

3. **Update `TaprootScript.h`:**
   - Add `AnchorCommitTapLeaf` struct definition inside `ailee::taproot`.
   - Add `TaprootScript` class definition with the `build_anchor_commit_tapleaf` static method inside `ailee::taproot`.

4. **Update `TaprootScript.cpp`:**
   - Implement `TaprootScript::build_anchor_commit_tapleaf(anchor_root, metadata_hash)` to construct the TapLeaf script: `OP_PUSH32 <anchor_root> OP_DROP OP_PUSH32 <metadata_hash> OP_DROP OP_TRUE`.
   - Compute the BIP342 leaf hash: `SHA256(tagged("TapLeaf") || 0xC0 || CompactSize(len(script)) || script)` using OpenSSL's SHA256 directly.

5. **Update `CMakeLists.txt`:**
   - Add `src/taproot/TaprootUtils.cpp` to `AILEE_CORE_SOURCES`.
   - Add a new executable target `anchor_commit_tapleaf_tests` for the new test file.

6. **Create tests:**
   - Create `tests/test_anchor_commit_tapleaf.cpp`.
   - Implement assertions to check script content and exact opcodes.
   - Check leaf hash against expected stable results.

7. **Pre-commit Steps:**
   - Complete pre-commit steps to make sure proper testing, verifications, reviews and reflections are done.

8. **Submit the change.**
   - Once all tests pass, submit the change with a descriptive commit message.
