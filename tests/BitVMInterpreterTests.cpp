#include "runtime/BitVMInterpreter.h"
#include <gtest/gtest.h>
#include <openssl/sha.h>

using namespace ailee::runtime;

TEST(BitVMInterpreterTest, HonestPathSha256) {
    BitVMInterpreter interpreter;

    // Honest path: hash a known value and verify it matches
    std::vector<uint8_t> preimage = {0x01, 0x02, 0x03};
    std::vector<uint8_t> expected_hash(SHA256_DIGEST_LENGTH);
    SHA256(preimage.data(), preimage.size(), expected_hash.data());

    // Script: OP_SHA256 OP_EQUALVERIFY
    std::vector<Instruction> script = {
        {OpCode::OP_SHA256, std::nullopt},
        {OpCode::OP_PUSH, expected_hash},
        {OpCode::OP_EQUALVERIFY, std::nullopt}
    };

    std::vector<std::vector<uint8_t>> initialStack = {preimage};
    auto state = interpreter.execute(script, initialStack);

    EXPECT_TRUE(state.execution_success);
    EXPECT_TRUE(state.stack.empty());
}

TEST(BitVMInterpreterTest, DishonestPathSha256) {
    BitVMInterpreter interpreter;

    // Dishonest path: hash an incorrect preimage
    std::vector<uint8_t> preimage = {0x01, 0x02, 0x03};
    std::vector<uint8_t> incorrect_hash(SHA256_DIGEST_LENGTH, 0x00); // wrong hash

    // Script: OP_SHA256 PUSH(wrong_hash) OP_EQUALVERIFY
    std::vector<Instruction> script = {
        {OpCode::OP_SHA256, std::nullopt},
        {OpCode::OP_PUSH, incorrect_hash},
        {OpCode::OP_EQUALVERIFY, std::nullopt}
    };

    std::vector<std::vector<uint8_t>> initialStack = {preimage};
    auto state = interpreter.execute(script, initialStack);

    EXPECT_FALSE(state.execution_success);
    EXPECT_EQ(state.error_message, "OP_EQUALVERIFY failed");
}

TEST(BitVMInterpreterTest, StackUnderflowOnEqualVerify) {
    BitVMInterpreter interpreter;

    // Script: OP_EQUALVERIFY (with only one item on stack)
    std::vector<Instruction> script = {
        {OpCode::OP_EQUALVERIFY, std::nullopt}
    };

    std::vector<std::vector<uint8_t>> initialStack = {{0x01}};
    auto state = interpreter.execute(script, initialStack);

    EXPECT_FALSE(state.execution_success);
    EXPECT_EQ(state.error_message, "Stack underflow on OP_EQUALVERIFY");
}

TEST(BitVMInterpreterTest, MalformedScriptInvalidPush) {
    BitVMInterpreter interpreter;

    // OP_PUSH (0x03) but only 1 byte follows
    std::vector<uint8_t> scriptBytes = {0x03, 0xff};

    bool threw = false;
    try {
        interpreter.parseScript(scriptBytes);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST(BitVMInterpreterTest, EdgeCaseAltStack) {
    BitVMInterpreter interpreter;

    // Script: OP_TOALTSTACK OP_FROMALTSTACK OP_EQUALVERIFY
    std::vector<Instruction> script = {
        {OpCode::OP_TOALTSTACK, std::nullopt},
        {OpCode::OP_FROMALTSTACK, std::nullopt},
        {OpCode::OP_EQUALVERIFY, std::nullopt}
    };

    std::vector<std::vector<uint8_t>> initialStack = {{0x01}, {0x01}};
    auto state = interpreter.execute(script, initialStack);

    EXPECT_TRUE(state.execution_success);
    EXPECT_TRUE(state.stack.empty());
}

TEST(BitVMInterpreterTest, ParseAndExecuteRawScript) {
    BitVMInterpreter interpreter;

    // OP_PUSH1 0x01, OP_PUSH1 0x01, OP_EQUALVERIFY
    // PUSH1 = 0x01
    // EQUALVERIFY = 0x88
    std::vector<uint8_t> scriptBytes = {
        0x01, 0x42, // PUSH 1 byte: 0x42
        0x01, 0x42, // PUSH 1 byte: 0x42
        0x88        // OP_EQUALVERIFY
    };

    auto state = interpreter.execute(scriptBytes);

    EXPECT_TRUE(state.execution_success);
    EXPECT_TRUE(state.stack.empty());
}

TEST(BitVMInterpreterTest, DupSwapDrop) {
    BitVMInterpreter interpreter;

    // Initial: [A]
    // DUP -> [A, A]
    // PUSH B -> [A, A, B]
    // SWAP -> [A, B, A]
    // DROP -> [A, B]
    // EQUAL -> [0] (since A != B)
    std::vector<Instruction> script = {
        {OpCode::OP_DUP, std::nullopt},
        {OpCode::OP_PUSH, std::vector<uint8_t>{0xbb}},
        {OpCode::OP_SWAP, std::nullopt},
        {OpCode::OP_DROP, std::nullopt},
        {OpCode::OP_EQUAL, std::nullopt}
    };

    std::vector<std::vector<uint8_t>> initialStack = {{0xaa}};
    auto state = interpreter.execute(script, initialStack);

    EXPECT_TRUE(state.execution_success);
    EXPECT_EQ(state.stack.size(), 1);
    EXPECT_TRUE(state.stack[0].empty()); // empty vector represents false in Bitcoin script
}

TEST(BitVMInterpreterTest, TestPushData124) {
    BitVMInterpreter interpreter;

    // OP_PUSHDATA1 0x02 0xAA 0xBB (push 2 bytes)
    std::vector<uint8_t> script1 = {0x4c, 0x02, 0xaa, 0xbb};
    auto state1 = interpreter.execute(script1);
    EXPECT_TRUE(state1.execution_success);
    EXPECT_EQ(state1.stack.size(), 1);
    EXPECT_EQ(state1.stack[0], (std::vector<uint8_t>{0xaa, 0xbb}));

    // OP_PUSHDATA2 0x03 0x00 0xAA 0xBB 0xCC (push 3 bytes, little endian length)
    std::vector<uint8_t> script2 = {0x4d, 0x03, 0x00, 0xaa, 0xbb, 0xcc};
    auto state2 = interpreter.execute(script2);
    EXPECT_TRUE(state2.execution_success);
    EXPECT_EQ(state2.stack.size(), 1);
    EXPECT_EQ(state2.stack[0], (std::vector<uint8_t>{0xaa, 0xbb, 0xcc}));

    // OP_PUSHDATA4 0x04 0x00 0x00 0x00 0xAA 0xBB 0xCC 0xDD (push 4 bytes, little endian length)
    std::vector<uint8_t> script3 = {0x4e, 0x04, 0x00, 0x00, 0x00, 0xaa, 0xbb, 0xcc, 0xdd};
    auto state3 = interpreter.execute(script3);
    EXPECT_TRUE(state3.execution_success);
    EXPECT_EQ(state3.stack.size(), 1);
    EXPECT_EQ(state3.stack[0], (std::vector<uint8_t>{0xaa, 0xbb, 0xcc, 0xdd}));
}

TEST(BitVMInterpreterTest, TestControlFlowIfElse) {
    BitVMInterpreter interpreter;

    // [True] OP_IF [Push 0x11] OP_ELSE [Push 0x22] OP_ENDIF
    std::vector<uint8_t> script = {
        0x51, // OP_1 (True)
        0x63, // OP_IF
        0x01, 0x11, // PUSH 0x11
        0x67, // OP_ELSE
        0x01, 0x22, // PUSH 0x22
        0x68  // OP_ENDIF
    };

    auto state = interpreter.execute(script);
    EXPECT_TRUE(state.execution_success);
    EXPECT_EQ(state.stack.size(), 1);
    EXPECT_EQ(state.stack[0], (std::vector<uint8_t>{0x11}));

    // [False] OP_IF [Push 0x11] OP_ELSE [Push 0x22] OP_ENDIF
    std::vector<uint8_t> script_false = {
        0x00, // OP_0 (False)
        0x63, // OP_IF
        0x01, 0x11, // PUSH 0x11
        0x67, // OP_ELSE
        0x01, 0x22, // PUSH 0x22
        0x68  // OP_ENDIF
    };

    auto state_false = interpreter.execute(script_false);
    EXPECT_TRUE(state_false.execution_success);
    EXPECT_EQ(state_false.stack.size(), 1);
    EXPECT_EQ(state_false.stack[0], (std::vector<uint8_t>{0x22}));
}

TEST(BitVMInterpreterTest, TestStepByStepExecution) {
    BitVMInterpreter interpreter;

    std::vector<Instruction> script = {
        {OpCode::OP_1, std::nullopt},
        {OpCode::OP_0, std::nullopt},
        {OpCode::OP_EQUAL, std::nullopt}
    };

    InterpreterState state;
    state.script = script;

    interpreter.step(state);
    EXPECT_EQ(state.ip, 1);
    EXPECT_EQ(state.stack.size(), 1);

    interpreter.step(state);
    EXPECT_EQ(state.ip, 2);
    EXPECT_EQ(state.stack.size(), 2);

    interpreter.step(state);
    EXPECT_EQ(state.ip, 3);
    EXPECT_EQ(state.stack.size(), 1); // 0 (False) on top
    EXPECT_TRUE(state.stack[0].empty());
}

TEST(BitVMInterpreterTest, TestVerifyRustProverOutput) {
    BitVMInterpreter interpreter;

    std::vector<uint8_t> state_root = {0xab, 0xcd, 0xef};

    InterpreterState state;
    state.stack = {state_root};
    state.execution_success = true;

    RustProverOutput rustOutput;
    rustOutput.state_root = state_root;
    rustOutput.is_valid = true;

    EXPECT_TRUE(interpreter.verifyRustProverOutput(state, rustOutput));

    // Make them mismatch
    rustOutput.state_root = {0xff, 0xff};
    EXPECT_FALSE(interpreter.verifyRustProverOutput(state, rustOutput));
}
