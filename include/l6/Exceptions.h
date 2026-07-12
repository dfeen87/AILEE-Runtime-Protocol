#pragma once

#include <stdexcept>
#include <string>

namespace ailee::l6 {

class DeterministicBackendException : public std::runtime_error {
public:
    explicit DeterministicBackendException(const std::string& msg)
        : std::runtime_error(msg) {}
};

class DeterministicError : public std::runtime_error {
public:
    explicit DeterministicError(const std::string& msg)
        : std::runtime_error(msg) {}
};

} // namespace ailee::l6
