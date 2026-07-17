#include "l3/NetworkDriver.h"
#include <iostream>
#include <cstring>
#include <curl/curl.h>
#include <json/json.h>

namespace ailee {
namespace l3 {

namespace {

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string do_rpc_call(const NetworkConfig& config, const std::string& method, const std::string& params_json = "[]") {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, config.rpc_url.c_str());

        std::string auth = config.rpc_user + ":" + config.rpc_password;
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string payload = "{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", \"method\": \"" + method + "\", \"params\": " + params_json + " }";
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream s(readBuffer);
    if (Json::parseFromStream(builder, s, &root, &errs) && root.isMember("result")) {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = ""; // compact
        return Json::writeString(writer, root["result"]);
    }

    return readBuffer; // fallback
}

} // anonymous namespace

NetworkSnapshot NetworkDriver::fetch_network_snapshot(const NetworkConfig& config, uint32_t step) {
    NetworkSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    // For tests/offline simulation without a real node, we can fallback if URL is empty or 'mock'
    if (config.rpc_url.empty() || config.rpc_url == "mock") {
        snap.block.height = step;
        snap.block.timestamp = 1600000000 + step;
        snap.mempool.transaction_count = 100 + step;
        snap.peer.peer_count = 8;
        snap.peer.active_connections = 4;
        return snap;
    }

    // Real RPC calls
    // 1. Get block hash by height (using step as height for the sequence)
    std::string block_hash_res = do_rpc_call(config, "getblockhash", "[" + std::to_string(step) + "]");

    // The result might be quoted "hash..." because json_write converts string to quoted string. Trim quotes.
    std::string block_hash = block_hash_res;
    if (block_hash.length() >= 2 && block_hash.front() == '"' && block_hash.back() == '"') {
        block_hash = block_hash.substr(1, block_hash.length() - 2);
    }

    // 2. Get block info
    std::string block_info_res = do_rpc_call(config, "getblock", "[\"" + block_hash + "\", 1]");
    snap.block = parse_network_block_snapshot(block_info_res);

    // 3. Get mempool info
    std::string mempool_info_res = do_rpc_call(config, "getmempoolinfo");
    snap.mempool = parse_network_mempool_snapshot(mempool_info_res);

    // 4. Get peer info
    std::string peer_info_res = do_rpc_call(config, "getpeerinfo");
    snap.peer = parse_network_peer_snapshot(peer_info_res);

    return snap;
}

NetworkRunSummary NetworkDriver::run_offline(
    const NetworkConfig& config,
    const identity::NodeId& node_id,
    uint32_t protocol_version
) {
    NetworkRunSummary summary;
    summary.total_steps_executed = 0;
    summary.final_network_height = 0;
    summary.envelopes_produced = 0;
    summary.sequence.count = 0;

    l2::DeterministicEngine engine;

    for (uint32_t step = 0; step < config.max_steps; ++step) {
        // 1. Fetch Network Snapshot
        NetworkSnapshot net_snap = fetch_network_snapshot(config, step);

        // 2. Bind L3 -> L2 Deterministically
        reflection::ReflectionSnapshot l2_reflection = bind_network_block(net_snap.block);
        l1::SettlementIngestion l2_settlement = bind_network_mempool(net_snap.mempool, net_snap.block);
        mesh::MeshCoherenceResult l2_coherence = bind_network_peer(net_snap.peer);

        // 3. Step Deterministic Engine
        l2::EngineStepResult step_result = engine.step(
            l2_reflection,
            l2_settlement,
            l2_coherence,
            node_id,
            protocol_version
        );

        // 4. Extract Envelope
        l2::ExecutionEnvelope envelope;
        std::memset(&envelope, 0, sizeof(envelope));

        envelope.context = l2::build_execution_context(
            node_id,
            step_result.new_state.epoch,
            step_result.new_state.state_root,
            l2_coherence
        );

        // Push to sequence
        summary.sequence.envelopes.push_back(envelope);
        summary.sequence.count++;

        summary.final_network_height = static_cast<uint32_t>(net_snap.block.height);
    }

    summary.total_steps_executed = config.max_steps;
    summary.envelopes_produced = static_cast<uint32_t>(summary.sequence.count);

    return summary;
}

} // namespace l3
} // namespace ailee
