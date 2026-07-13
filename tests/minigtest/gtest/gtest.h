#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstdint>
#include <array>
#include <iomanip>

namespace testing {

struct TestInfo {
    std::string suite;
    std::string name;
    void (*func)();
};

void RegisterTest(const std::string& suite, const std::string& name, void (*func)());
int RunAllTests();
inline void InitGoogleTest(int* argc, char** argv) {
    (void)argc;
    (void)argv;
}

namespace detail {

struct AbortTest : public std::exception {
    const char* what() const noexcept override {
        return "test aborted";
    }
};

struct TestState {
    int failures = 0;
};

TestState& CurrentState();
void ReportFailure(const std::string& message, const char* file, int line);

// -----------------------------
// Custom ToString overloads
// -----------------------------

inline std::string ToString(const std::string& value) {
    return "\"" + value + "\"";
}

inline std::string ToString(const char* value) {
    return value ? "\"" + std::string(value) + "\"" : "<null>";
}

inline std::string ToString(const std::vector<uint8_t>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << std::hex << (int)vec[i];
        if (i < vec.size() - 1) oss << ", ";
    }
    oss << std::dec << "]";
    return oss.str();
}

// NEW: hex printer for std::array<unsigned char, N>
template <size_t N>
inline std::string ToString(const std::array<unsigned char, N>& arr) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : arr) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

template <typename T, typename std::enable_if_t<!std::is_enum_v<T>, int> = 0>
std::string ToString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
std::string ToString(const T& value) {
    using Underlying = std::underlying_type_t<T>;
    return std::to_string(static_cast<Underlying>(value));
}

template <typename A, typename B>
void ExpectBinary(bool result,
                  const A& lhs,
                  const B& rhs,
                  const char* op,
                  const char* expr_lhs,
                  const char* expr_rhs,
                  const char* file,
                  int line,
                  bool fatal) {
    if (result) {
        return;
    }

    std::ostringstream oss;
    oss << "Expectation failed: (" << expr_lhs << ") " << op << " (" << expr_rhs
        << ") with values " << ToString(lhs) << " and " << ToString(rhs);
    ReportFailure(oss.str(), file, line);

    if (fatal) {
        throw AbortTest();
    }
}

inline void ExpectTrue(bool result,
                       const char* expr,
                       const char* file,
                       int line,
                       bool fatal) {
    if (result) {
        return;
    }
    std::ostringstream oss;
    oss << "Expectation failed: " << expr << " is false";
    ReportFailure(oss.str(), file, line);

    if (fatal) {
        throw AbortTest();
    }
}

} // namespace detail
} // namespace testing

// -----------------------------
// Test registration macros
// -----------------------------

#define RUN_ALL_TESTS() ::testing::RunAllTests()

#define TEST(SUITE, NAME)                                                     \
    static void SUITE##_##NAME##_Test();                                      \
    namespace {                                                               \
    const bool SUITE##_##NAME##_registered = []() {                           \
        ::testing::RegisterTest(#SUITE, #NAME, &SUITE##_##NAME##_Test);       \
        return true;                                                          \
    }();                                                                      \
    }                                                                         \
    static void SUITE##_##NAME##_Test()

// -----------------------------
// Assertions
// -----------------------------

#define EXPECT_TRUE(CONDITION)                                                \
    ::testing::detail::ExpectTrue(static_cast<bool>(CONDITION),               \
                                  #CONDITION, __FILE__, __LINE__, false)

#define EXPECT_FALSE(CONDITION)                                               \
    ::testing::detail::ExpectTrue(!static_cast<bool>(CONDITION),              \
                                  "!(" #CONDITION ")", __FILE__, __LINE__, false)

#define ASSERT_TRUE(CONDITION)                                                \
    ::testing::detail::ExpectTrue(static_cast<bool>(CONDITION),               \
                                  #CONDITION, __FILE__, __LINE__, true)

#define EXPECT_EQ(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) == (B), (A), (B), "==", #A, #B,        \
                                    __FILE__, __LINE__, false)

#define EXPECT_NE(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) != (B), (A), (B), "!=", #A, #B,        \
                                    __FILE__, __LINE__, false)

#define EXPECT_LT(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) < (B), (A), (B), "<", #A, #B,          \
                                    __FILE__, __LINE__, false)

#define EXPECT_GT(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) > (B), (A), (B), ">", #A, #B,          \
                                    __FILE__, __LINE__, false)

// NEW: >= and <= support
#define EXPECT_GE(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) >= (B), (A), (B), ">=", #A, #B,        \
                                    __FILE__, __LINE__, false)

#define EXPECT_LE(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) <= (B), (A), (B), "<=", #A, #B,        \
                                    __FILE__, __LINE__, false)

#define ASSERT_EQ(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) == (B), (A), (B), "==", #A, #B,        \
                                    __FILE__, __LINE__, true)

#define ASSERT_NE(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) != (B), (A), (B), "!=", #A, #B,        \
                                    __FILE__, __LINE__, true)

#define EXPECT_DOUBLE_EQ(A, B)                                                \
    ::testing::detail::ExpectBinary(std::abs((A) - (B)) < 1e-9, (A), (B), "==", #A, #B, \
                                    __FILE__, __LINE__, false)

#define EXPECT_NEAR(A, B, C)                                                  \
    ::testing::detail::ExpectBinary(std::abs((A) - (B)) <= (C), (A), (B), "NEAR", #A, #B, \
                                    __FILE__, __LINE__, false)

namespace testing {
class Test {
public:
    virtual ~Test() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};
}

#define TEST_F(TEST_FIXTURE, TEST_NAME)                                       \
    class TEST_FIXTURE##_##TEST_NAME##_Test : public TEST_FIXTURE {           \
    public:                                                                   \
        void TestBody();                                                      \
        void RunTest() {                                                      \
            SetUp();                                                          \
            try {                                                             \
                TestBody();                                                   \
            } catch(...) {}                                                   \
            TearDown();                                                       \
        }                                                                     \
    };                                                                        \
    static void TEST_FIXTURE##_##TEST_NAME##_Run() {                          \
        TEST_FIXTURE##_##TEST_NAME##_Test test;                               \
        test.RunTest();                                                       \
    }                                                                         \
    namespace {                                                               \
        const bool TEST_FIXTURE##_##TEST_NAME##_registered = []() {           \
            ::testing::RegisterTest(#TEST_FIXTURE, #TEST_NAME,                \
                                    &TEST_FIXTURE##_##TEST_NAME##_Run);       \
            return true;                                                      \
        }();                                                                  \
    }                                                                         \
    void TEST_FIXTURE##_##TEST_NAME##_Test::TestBody()
