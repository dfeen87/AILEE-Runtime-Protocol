// SPDX-License-Identifier: MIT
// crypto_utils.h — Shared cryptographic utility functions for AILEE-Core
//
// Provides a single, canonical SHA-256 hex-encoding helper used across
// multiple translation units (AmbientAI.cpp, config_hot_reload.cpp, etc.)
// to avoid duplicate implementations.

#pragma once

#include <string>
#include <vector>
#include <openssl/sha.h>

namespace ailee::crypto {

/**
 * Compute the SHA-256 hash of the given byte string and return it
 * as a lower-case hexadecimal string (64 characters).
 */
inline std::string sha256_hex(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    std::string out;
    out.reserve(2 * SHA256_DIGEST_LENGTH);
    static const char* kHex = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out.push_back(kHex[hash[i] >> 4]);
        out.push_back(kHex[hash[i] & 0x0F]);
    }
    return out;
}

/**
 * Compute the SHA-256 hash of the given byte vector and return it
 * as a lower-case hexadecimal string (64 characters).
 */
inline std::string sha256_hex(const std::vector<uint8_t>& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    std::string out;
    out.reserve(2 * SHA256_DIGEST_LENGTH);
    static const char* kHex = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out.push_back(kHex[hash[i] >> 4]);
        out.push_back(kHex[hash[i] & 0x0F]);
    }
    return out;
}

} // namespace ailee::crypto
