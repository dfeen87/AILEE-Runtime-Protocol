/**
 * AILEE Bitcoin RPC Client (Production Hardened)
 *
 * A thread-safe, fault-tolerant HTTP client for the Bitcoin Core JSON-RPC interface.
 * Features:
 * - Exponential Backoff Retries
 * - Thread-safe request locking (Mutex)
 * - Automatic Basic Auth handling
 * - Configurable Timeouts
 *
 * Dependencies: libcurl
 *
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_BITCOIN_RPC_CLIENT_H
#define AILEE_BITCOIN_RPC_CLIENT_H

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp> // Requires libcurl
#include "RpcSandboxSimulator.h"

namespace ailee {

// Enum to define explicit deterministic error states
enum class RpcErrorState {
    NONE,
    ENDPOINT_NOT_FOUND,
    INVALID_JSON_RESPONSE,
    CRITICAL_NULL_FIELD,
    TIMEOUT
};

class BitcoinRPCClient {
public:
    BitcoinRPCClient(const std::string& rpcUser,
                     const std::string& rpcPassword,
                     const std::string& rpcUrl = "http://127.0.0.1:8332",
                     bool simulationMode = false)
        : rpcUser_(rpcUser), rpcPassword_(rpcPassword), rpcUrl_(rpcUrl),
          simulationMode_(simulationMode), lastError_(RpcErrorState::NONE) {
        
        // Initialize CURL globally (should be done once per app, but safe here with flags)
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~BitcoinRPCClient() {
        curl_global_cleanup();
    }

    // Retrieves the last deterministic error state
    RpcErrorState getLastError() const {
        return lastError_;
    }

    // Access to sandbox simulator if in simulation mode
    RpcSandboxSimulator& getSimulator() {
        return simulator_;
    }

    /**
     * Broadcast a Raw Transaction (sendrawtransaction)
     * Used by the Bridge to settle Gold conversions or Recovery flows.
     */
    bool broadcastCheckpoint(const std::string& hexTx) {
        if (lastError_ != RpcErrorState::NONE) {
            std::cerr << "[RPC] Client in deterministic error state. Broadcast blocked." << std::endl;
            return false;
        }

        // Construct JSON-RPC payload
        std::string payload = buildJsonPayload("sendrawtransaction", "\"" + hexTx + "\"");
        
        std::string response;
        if (executeRPC("sendrawtransaction", payload, response)) {
            std::cout << "[RPC] TX Broadcast Success. Response: " << response << std::endl;
            return true;
        }
        
        std::cerr << "[RPC] TX Broadcast Failed." << std::endl;
        return false;
    }

    /**
     * Get Block Count
     * Used to verify synchronization and calculating VDF maturity.
     */
    long getBlockCount() {
        if (lastError_ != RpcErrorState::NONE) return -1;

        std::string payload = buildJsonPayload("getblockcount", "");
        std::string response;
        
        if (executeRPC("getblockcount", payload, response)) {
            try {
                auto j = nlohmann::json::parse(response);
                if (j.contains("result") && !j["result"].is_null()) {
                    return j["result"].get<long>();
                } else if (j.contains("result") && j["result"].is_null()) {
                    lastError_ = RpcErrorState::CRITICAL_NULL_FIELD;
                    std::cerr << "[RPC] Deterministic Failure: Critical null field in block count result." << std::endl;
                    return -1;
                }
            } catch (...) {
                lastError_ = RpcErrorState::INVALID_JSON_RESPONSE;
                std::cerr << "[RPC] Failed to parse block count." << std::endl;
            }
        }
        return -1;
    }

    long getTxConfirmations(const std::string& txId) {
        if (lastError_ != RpcErrorState::NONE) return -1;

        std::string params = "\"" + txId + "\", true";
        std::string payload = buildJsonPayload("getrawtransaction", params);
        std::string response;

        if (executeRPC("getrawtransaction", payload, response)) {
            try {
                auto j = nlohmann::json::parse(response);
                if (j.contains("result") && !j["result"].is_null()) {

                    // Reject critical nulls (e.g. txid, blockhash if expected)
                    if (j["result"].contains("txid") && j["result"]["txid"].is_null()) {
                        lastError_ = RpcErrorState::CRITICAL_NULL_FIELD;
                        std::cerr << "[RPC] Deterministic Failure: Null txid field." << std::endl;
                        return -1;
                    }

                    // Coerce optional fields
                    long timeReceived = 0;
                    if (j["result"].contains("time_received")) {
                        if (j["result"]["time_received"].is_null()) {
                            timeReceived = 0; // Coercion
                        } else {
                            timeReceived = j["result"]["time_received"].get<long>();
                        (void)timeReceived;
                        }
                    }

                    if (j["result"].contains("confirmations") && !j["result"]["confirmations"].is_null()) {
                        return j["result"]["confirmations"].get<long>();
                    } else {
                        return 0; // Or handle as null gracefully depending on protocol
                    }
                }
            } catch (...) {
                lastError_ = RpcErrorState::INVALID_JSON_RESPONSE;
                std::cerr << "[RPC] Failed to parse getrawtransaction response for " << txId << std::endl;
            }
        }
        return -1;
    }

private:
    std::string rpcUser_;
    std::string rpcPassword_;
    std::string rpcUrl_;
    bool simulationMode_;
    RpcErrorState lastError_;
    RpcSandboxSimulator simulator_;
    std::mutex clientMutex_; // Ensures thread safety for CURL calls

    // Maximum retries before giving up
    const int MAX_RETRIES = 3;
    const int TIMEOUT_SECONDS = 10;

    // Helper: Build JSON-RPC string
    std::string buildJsonPayload(const std::string& method, const std::string& params) {
        std::stringstream ss;
        ss << "{\"jsonrpc\": \"1.0\", \"id\": \"ailee-core\", \"method\": \"" 
           << method << "\", \"params\": [" << params << "]}";
        return ss.str();
    }

    // CURL Write Callback
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Core execution logic with Retries and Locking
    bool executeRPC(const std::string& method, const std::string& postData, std::string& responseBuffer) {
        std::lock_guard<std::mutex> lock(clientMutex_); // Lock this thread
        
        if (simulationMode_) {
            long http_code = simulator_.execute(method, postData, responseBuffer);
            if (http_code == 404) {
                lastError_ = RpcErrorState::ENDPOINT_NOT_FOUND;
                std::cerr << "[RPC] Sandbox Simulated 404: Endpoint not found." << std::endl;
                return false;
            }
            return http_code == 200;
        }

        int attempts = 0;
        bool success = false;

        while (attempts < MAX_RETRIES && !success) {
            CURL* curl = curl_easy_init();
            if (!curl) return false;

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "content-type: text/plain;");
            
            curl_easy_setopt(curl, CURLOPT_URL, rpcUrl_.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
            curl_easy_setopt(curl, CURLOPT_USERPWD, (rpcUser_ + ":" + rpcPassword_).c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECONDS);

            CURLcode res = curl_easy_perform(curl);
            
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (res == CURLE_OK && http_code == 200) {
                success = true;
            } else if (http_code == 404) {
                // Deterministic 404 Handling: Zero retries
                lastError_ = RpcErrorState::ENDPOINT_NOT_FOUND;
                std::cerr << "[RPC] Deterministic Failure: HTTP 404 Endpoint Not Found. Aborting retries." << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return false;
            } else {
                attempts++;
                std::cerr << "[RPC] Request failed (Attempt " << attempts << "/" << MAX_RETRIES 
                          << "). HTTP: " << http_code << " Curl: " << curl_easy_strerror(res) << std::endl;
                
                // Exponential backoff wait
                if (attempts < MAX_RETRIES) {
                    std::this_thread::sleep_for(std::chrono::seconds(attempts * 2));
                }
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        return success;
    }
};

} // namespace ailee

#endif // AILEE_BITCOIN_RPC_CLIENT_H
