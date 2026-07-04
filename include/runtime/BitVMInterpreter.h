#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <optional>

namespace ailee {
namespace runtime {

// Restricted set of Bitcoin opcodes supported by the BitVM interpreter
enum class OpCode : uint8_t {
    OP_0 = 0x00,
    OP_1 = 0x51,
    OP_NOP = 0x61,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
    OP_RETURN = 0x6a,
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_SWAP = 0x7c,
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,
    OP_SHA256 = 0xa8,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,

    // Pseudo opcode to represent push operations
    OP_PUSH = 0xff
};

// Represents a parsed script instruction
struct Instruction {
    OpCode opcode;
    std::optional<std::vector<uint8_t>> data; // Only present for OP_PUSH
};

// Execution state for the interpreter
struct InterpreterState {
    std::vector<std::vector<uint8_t>> stack;
    std::vector<std::vector<uint8_t>> alt_stack;
    std::vector<Instruction> script;
    size_t ip = 0; // Instruction Pointer
    bool execution_success = true;
    std::string error_message = "";

    // Flow control state
    std::vector<bool> if_stack;
    bool executing = true;
};

// Wiring struct for Rust Prover Output
struct RustProverOutput {
    std::vector<uint8_t> proof_root;
    std::vector<uint8_t> state_root;
    std::vector<uint8_t> challenge_root;
    bool is_valid = false;
};

class BitVMInterpreter {
public:
    BitVMInterpreter() = default;

    // Parse a raw script byte array into instructions
    std::vector<Instruction> parseScript(const std::vector<uint8_t>& scriptBytes) const;

    // Execute a parsed script with an initial stack
    InterpreterState execute(const std::vector<Instruction>& script, const std::vector<std::vector<uint8_t>>& initialStack = {}) const;

    // Execute a raw script
    InterpreterState execute(const std::vector<uint8_t>& scriptBytes, const std::vector<std::vector<uint8_t>>& initialStack = {}) const {
        return execute(parseScript(scriptBytes), initialStack);
    }

    // Execute a single step in the interpreter
    void step(InterpreterState& state) const;

    // Wire to Rust Prover outputs
    bool verifyRustProverOutput(InterpreterState& state, const RustProverOutput& rustOutput) const;
};

} // namespace runtime
} // namespace ailee
