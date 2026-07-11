#include "l6/BitcoinClock.h"
#include "Logging.h"
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <stdexcept>
#include <sstream>

namespace ailee::l6 {

namespace {

size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch(std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

} // namespace

BitcoinClock::BitcoinClock(const BitcoinRpcConfig& config) : config_(config) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

BitcoinClock::~BitcoinClock() {
    curl_global_cleanup();
}

std::string BitcoinClock::make_rpc_call(const std::string& method, const std::string& params) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl");
    }

    std::string response_string;
    std::string payload = "{\"jsonrpc\": \"1.0\", \"id\": \"curltest\", \"method\": \"" + method + "\", \"params\": " + params + "}";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "content-type: text/plain;");

    curl_easy_setopt(curl, CURLOPT_URL, config_.url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    if (!config_.user.empty() && !config_.pass.empty()) {
        std::string auth = config_.user + ":" + config_.pass;
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    return response_string;
}

ClockSnapshot BitcoinClock::get_snapshot() const {
    auto logger = ailee::log::getLogger("L6");

    try {
        // 1. Get block count
        std::string height_resp = make_rpc_call("getblockcount", "[]");
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(height_resp, root) || root["error"].type() != Json::nullValue) {
            throw std::runtime_error("Failed to parse getblockcount response");
        }
        uint64_t height = root["result"].asUInt64();

        // 2. Get block hash for the height
        std::string hash_resp = make_rpc_call("getblockhash", "[" + std::to_string(height) + "]");
        if (!reader.parse(hash_resp, root) || root["error"].type() != Json::nullValue) {
            throw std::runtime_error("Failed to parse getblockhash response");
        }
        std::string block_hash = root["result"].asString();

        // 3. Get block header for the hash to get mediantime
        std::string header_resp = make_rpc_call("getblockheader", "[\"" + block_hash + "\"]");
        if (!reader.parse(header_resp, root) || root["error"].type() != Json::nullValue) {
            throw std::runtime_error("Failed to parse getblockheader response");
        }
        uint64_t mtp = root["result"]["mediantime"].asUInt64();

        return {height, mtp, block_hash, "bitcoin-rpc"};
    } catch (const std::exception& e) {
        LOG_WARN(logger, "BitcoinClock error: " + std::string(e.what()));
        throw;
    }
}

} // namespace ailee::l6
