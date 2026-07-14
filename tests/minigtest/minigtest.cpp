#include "gtest/gtest.h"

#include <iostream>
#include <vector>

namespace testing {

namespace {
std::vector<TestInfo>& Registry() {
    static std::vector<TestInfo> tests;
    return tests;
}
} // namespace

void RegisterTest(const std::string& suite, const std::string& name, void (*func)()) {
    Registry().push_back(TestInfo{suite, name, func});
}

namespace detail {

TestState& CurrentState() {
    static TestState state;
    return state;
}

void ReportFailure(const std::string& message, const char* file, int line) {
    ++CurrentState().failures;
    std::cerr << file << ":" << line << ": " << message << std::endl;
}

} // namespace detail

int RunAllTests() {
    int failed_suites = 0;

    for (const auto& test : Registry()) {
        detail::CurrentState().failures = 0;
        try {
            test.func();
        } catch (const detail::AbortTest&) {
            // Fatal assertion already recorded; continue to next test.
        } catch (const std::exception& ex) {
            detail::ReportFailure(std::string("Unhandled exception: ") + ex.what(),
                                  __FILE__,
                                  __LINE__);
        } catch (...) {
            detail::ReportFailure("Unhandled non-standard exception",
                                  __FILE__,
                                  __LINE__);
        }

        if (detail::CurrentState().failures > 0) {
            ++failed_suites;
            std::cerr << "[  FAILED  ] " << test.suite << "." << test.name << std::endl;
        } else {
            std::cout << "[       OK ] " << test.suite << "." << test.name << std::endl;
        }
    }

    if (failed_suites == 0) {
        std::cout << "[  PASSED  ] All tests passed." << std::endl;
    } else {
        std::cerr << "[  FAILED  ] " << failed_suites << " test(s)." << std::endl;
    }

    return failed_suites;
}

} // namespace testing
