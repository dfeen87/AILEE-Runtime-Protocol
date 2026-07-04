#include "runtime/BitVMInterpreter.h"
#include <openssl/sha.h>
#include <iostream>

namespace ailee {
namespace runtime {

std::vector<Instruction> BitVMInterpreter::parseScript(const std::vector<uint8_t>& scriptBytes) const {
    std::vector<Instruction> instructions;
    size_t i = 0;
    while (i < scriptBytes.size()) {
        uint8_t op = scriptBytes[i];

        if (op > 0x00 && op <= 0x4b) {
            // OP_PUSH (1 to 75 bytes)
            size_t push_len = op;
            if (i + 1 + push_len > scriptBytes.size()) {
                throw std::runtime_error("Invalid OP_PUSH data length");
            }
            std::vector<uint8_t> data(scriptBytes.begin() + i + 1, scriptBytes.begin() + i + 1 + push_len);
            instructions.push_back({OpCode::OP_PUSH, data});
            i += 1 + push_len;
        } else if (op == 0x4c) {
            // OP_PUSHDATA1
            if (i + 1 >= scriptBytes.size()) throw std::runtime_error("Invalid OP_PUSHDATA1 length");
            size_t push_len = scriptBytes[i + 1];
            if (i + 2 + push_len > scriptBytes.size()) throw std::runtime_error("Invalid OP_PUSHDATA1 data length");
            std::vector<uint8_t> data(scriptBytes.begin() + i + 2, scriptBytes.begin() + i + 2 + push_len);
            instructions.push_back({OpCode::OP_PUSH, data});
            i += 2 + push_len;
        } else if (op == 0x4d) {
            // OP_PUSHDATA2
            if (i + 2 >= scriptBytes.size()) throw std::runtime_error("Invalid OP_PUSHDATA2 length");
            size_t push_len = scriptBytes[i + 1] | (scriptBytes[i + 2] << 8); // Little endian
            if (i + 3 + push_len > scriptBytes.size()) throw std::runtime_error("Invalid OP_PUSHDATA2 data length");
            std::vector<uint8_t> data(scriptBytes.begin() + i + 3, scriptBytes.begin() + i + 3 + push_len);
            instructions.push_back({OpCode::OP_PUSH, data});
            i += 3 + push_len;
        } else if (op == 0x4e) {
            // OP_PUSHDATA4
            if (i + 4 >= scriptBytes.size()) throw std::runtime_error("Invalid OP_PUSHDATA4 length");
            size_t push_len = static_cast<size_t>(scriptBytes[i + 1]) |
                              (static_cast<size_t>(scriptBytes[i + 2]) << 8) |
                              (static_cast<size_t>(scriptBytes[i + 3]) << 16) |
                              (static_cast<size_t>(scriptBytes[i + 4]) << 24); // Little endian
            if (i + 5 + push_len > scriptBytes.size()) throw std::runtime_error("Invalid OP_PUSHDATA4 data length");
            std::vector<uint8_t> data(scriptBytes.begin() + i + 5, scriptBytes.begin() + i + 5 + push_len);
            instructions.push_back({OpCode::OP_PUSH, data});
            i += 5 + push_len;
        } else {
            // Other opcodes (reject 0xff, which collides with the internal OP_PUSH marker)
            if (op == 0xff) {
                throw std::runtime_error("Unsupported opcode 0xff");
            }
            instructions.push_back({static_cast<OpCode>(op), std::nullopt});
            i++;
        }
    }
    return instructions;
}

InterpreterState BitVMInterpreter::execute(const std::vector<Instruction>& script, const std::vector<std::vector<uint8_t>>& initialStack) const {
    InterpreterState state;
    state.stack = initialStack;
    state.script = script;
    state.ip = 0;

    while (state.execution_success && state.ip < state.script.size()) {
        step(state);
    }
    return state;
}

void BitVMInterpreter::step(InterpreterState& state) const {
    if (!state.execution_success || state.ip >= state.script.size()) {
        return; // Execution finished or failed
    }

    const auto& inst = state.script[state.ip];
    state.ip++; // Advance IP before execution

    // Check if we are currently in a non-executing branch
    if (inst.opcode == OpCode::OP_IF || inst.opcode == OpCode::OP_NOTIF) {
        bool condition = false;
        if (state.executing) {
            if (state.stack.empty()) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_IF/OP_NOTIF";
                return;
            }
            auto top = state.stack.back();
            state.stack.pop_back();
            condition = false;
            for (size_t j = 0; j < top.size(); ++j) {
                if (top[j] != 0) {
                    condition = !(j == top.size() - 1 && top[j] == 0x80);
                    break;
                }
            }
            if (inst.opcode == OpCode::OP_NOTIF) {
                condition = !condition;
            }
        }
        state.if_stack.push_back(condition);
        if (state.executing && !condition) {
            state.executing = false;
        }
        return;
    } else if (inst.opcode == OpCode::OP_ELSE) {
        if (state.if_stack.empty()) {
            state.execution_success = false;
            state.error_message = "OP_ELSE without OP_IF";
            return;
        }

        bool current_cond = state.if_stack.back();
        state.if_stack.back() = !current_cond;

        // Re-evaluate if we are executing
        state.executing = true;
        for (bool cond : state.if_stack) {
            if (!cond) {
                state.executing = false;
                break;
            }
        }
        return;
    } else if (inst.opcode == OpCode::OP_ENDIF) {
        if (state.if_stack.empty()) {
            state.execution_success = false;
            state.error_message = "OP_ENDIF without OP_IF";
            return;
        }
        state.if_stack.pop_back();

        // Re-evaluate if we are executing
        state.executing = true;
        for (bool cond : state.if_stack) {
            if (!cond) {
                state.executing = false;
                break;
            }
        }
        return;
    }

    // Skip if we are in a false branch
    if (!state.executing) {
        return;
    }

    switch (inst.opcode) {
        case OpCode::OP_PUSH: {
            if (inst.data.has_value()) {
                state.stack.push_back(inst.data.value());
            } else {
                state.execution_success = false;
                state.error_message = "OP_PUSH missing data";
            }
            break;
        }
        case OpCode::OP_0: {
            state.stack.push_back({});
            break;
        }
        case OpCode::OP_1: {
            state.stack.push_back({1});
            break;
        }
        case OpCode::OP_NOP: {
            break;
        }
        case OpCode::OP_DROP: {
            if (state.stack.empty()) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_DROP";
                return;
            }
            state.stack.pop_back();
            break;
        }
        case OpCode::OP_DUP: {
            if (state.stack.empty()) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_DUP";
                return;
            }
            state.stack.push_back(state.stack.back());
            break;
        }
        case OpCode::OP_SWAP: {
            if (state.stack.size() < 2) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_SWAP";
                return;
            }
            auto top = state.stack.back();
            state.stack.pop_back();
            auto next = state.stack.back();
            state.stack.pop_back();
            state.stack.push_back(top);
            state.stack.push_back(next);
            break;
        }
        case OpCode::OP_EQUAL: {
            if (state.stack.size() < 2) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_EQUAL";
                return;
            }
            auto a = state.stack.back();
            state.stack.pop_back();
            auto b = state.stack.back();
            state.stack.pop_back();
            if (a == b) {
                state.stack.push_back({1});
            } else {
                state.stack.push_back({});
            }
            break;
        }
        case OpCode::OP_EQUALVERIFY: {
            if (state.stack.size() < 2) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_EQUALVERIFY";
                return;
            }
            auto a = state.stack.back();
            state.stack.pop_back();
            auto b = state.stack.back();
            state.stack.pop_back();
            if (a != b) {
                state.execution_success = false;
                state.error_message = "OP_EQUALVERIFY failed";
                return;
            }
            break;
        }
        case OpCode::OP_VERIFY: {
            if (state.stack.empty()) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_VERIFY";
                return;
            }
            auto a = state.stack.back();
            state.stack.pop_back();

            bool is_true = false;
            for (size_t j = 0; j < a.size(); ++j) {
                if (a[j] != 0) {
                    is_true = !(j == a.size() - 1 && a[j] == 0x80);
                    break;
                }
            }
            if (!is_true) {
                state.execution_success = false;
                state.error_message = "OP_VERIFY failed";
                return;
            }
            break;
        }
        case OpCode::OP_SHA256: {
            if (state.stack.empty()) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_SHA256";
                return;
            }
            auto data = state.stack.back();
            state.stack.pop_back();

            std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
            SHA256(data.data(), data.size(), hash.data());

            state.stack.push_back(hash);
            break;
        }
        case OpCode::OP_TOALTSTACK: {
            if (state.stack.empty()) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_TOALTSTACK";
                return;
            }
            state.alt_stack.push_back(state.stack.back());
            state.stack.pop_back();
            break;
        }
        case OpCode::OP_FROMALTSTACK: {
            if (state.alt_stack.empty()) {
                state.execution_success = false;
                state.error_message = "Alt-stack underflow on OP_FROMALTSTACK";
                return;
            }
            state.stack.push_back(state.alt_stack.back());
            state.alt_stack.pop_back();
            break;
        }
        case OpCode::OP_RETURN: {
            state.execution_success = false;
            state.error_message = "OP_RETURN encountered";
            return;
        }
        // Basic signature opcodes just verify as true for now if stack sizes match, to act as a placeholder. In reality this requires secp256k1 integration.
        case OpCode::OP_CHECKSIG: {
            if (state.stack.size() < 2) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_CHECKSIG";
                return;
            }
            state.stack.pop_back(); // pubkey
            state.stack.pop_back(); // sig
            state.stack.push_back({1}); // push true
            break;
        }
        case OpCode::OP_CHECKSIGVERIFY: {
            if (state.stack.size() < 2) {
                state.execution_success = false;
                state.error_message = "Stack underflow on OP_CHECKSIGVERIFY";
                return;
            }
            state.stack.pop_back(); // pubkey
            state.stack.pop_back(); // sig
            break; // verify true
        }
        default: {
            state.execution_success = false;
            state.error_message = "Unsupported opcode";
            return;
        }
    }
}

bool BitVMInterpreter::verifyRustProverOutput(InterpreterState& state, const RustProverOutput& rustOutput) const {
    // Wiring the Rust Prover Outputs to the BitVM interpreter.
    // In an actual integration, we would initialize the stack with the state_root / proof_root.
    // This is a minimal stub to show deterministic commitment linkage.
    if (!state.execution_success) return false;

    // We expect the L2 state root to match what's on the top of the stack if it was a valid challenge-response
    if (!state.stack.empty()) {
        auto top = state.stack.back();
        if (top == rustOutput.state_root) {
            return true;
        }
    }

    // If the stack is empty, we cannot verify roots deterministically here.
    // Avoid treating rustOutput.is_valid as sufficient proof of equivalence.
    // (Callers that want "empty stack implies success" should enforce that policy externally.)

    return false;
}

} // namespace runtime
} // namespace ailee
