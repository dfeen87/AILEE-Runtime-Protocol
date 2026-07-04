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

#define ASSERT_EQ(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) == (B), (A), (B), "==", #A, #B,        \
                                    __FILE__, __LINE__, true)

#define ASSERT_NE(A, B)                                                       \
    ::testing::detail::ExpectBinary((A) != (B), (A), (B), "!=", #A, #B,        \
                                    __FILE__, __LINE__, true)
