#include "ailee_api.hpp"
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include "build_metadata.hpp"

using json = nlohmann::json;

namespace {

ailee::BitcoinSnapshot parse_snapshot(const json& j) {
    ailee::BitcoinSnapshot snap;
    snap.height = j.value("height", 0);

    if (j.contains("price_context")) {
        auto& pj = j["price_context"];
        snap.price_context.spot_price = pj.value("spot_price", 0.0);
        snap.price_context.daily_change_pct = pj.value("daily_change_pct", 0.0);
        snap.price_context.volatility_index = pj.value("volatility_index", 0.0);
    }

    if (j.contains("mempool_stats")) {
        auto& mj = j["mempool_stats"];
        snap.mempool_stats.size_bytes = mj.value("size_bytes", 0ULL);
        snap.mempool_stats.transaction_count = mj.value("transaction_count", 0U);
        snap.mempool_stats.avg_fee_rate = mj.value("avg_fee_rate", 0.0);
        snap.mempool_stats.high_fee_band = mj.value("high_fee_band", 0.0);
        snap.mempool_stats.low_fee_band = mj.value("low_fee_band", 0.0);
    }

    snap.current_fee_rate = j.value("current_fee_rate", 0.0);
    snap.hashrate_estimate = j.value("hashrate_estimate", 0.0);
    snap.recent_volatility = j.value("recent_volatility", 0.0);
    snap.dominance_or_regime_tag = j.value("dominance_or_regime_tag", "");
    snap.time_since_last_block = j.value("time_since_last_block", 0U);
    snap.block_interval_avg = j.value("block_interval_avg", 600U);

    return snap;
}

ailee::Policy parse_policy(const json& j) {
    ailee::Policy pol;
    pol.max_acceptable_risk_score = j.value("max_acceptable_risk_score", 5.0);
    pol.max_fee_rate_sats_per_vbyte = j.value("max_fee_rate_sats_per_vbyte", 100.0);
    pol.min_confidence_required = j.value("min_confidence_required", 0.8);
    if (j.contains("allowed_regimes")) {
        for (const auto& item : j["allowed_regimes"]) {
            pol.allowed_regimes.push_back(item.get<std::string>());
        }
    } else {
         pol.allowed_regimes = {"neutral", "risk-on"};
    }
    pol.block_interval_tolerance_sec = j.value("block_interval_tolerance_sec", 300.0);
    return pol;
}

std::string risk_band_to_string(ailee::RiskBand band) {
    switch (band) {
        case ailee::RiskBand::Low: return "Low";
        case ailee::RiskBand::Medium: return "Medium";
        case ailee::RiskBand::High: return "High";
        case ailee::RiskBand::Extreme: return "Extreme";
        default: return "Unknown";
    }
}

json risk_score_to_json(const ailee::RiskScore& score) {
    return {
        {"value", score.value},
        {"band", risk_band_to_string(score.band)}
    };
}

json posture_report_to_json(const ailee::PostureReport& report) {
    return {
        {"risk", risk_score_to_json(report.risk)},
        {"regime", report.regime},
        {"summary", report.summary},
        {"confidence", report.confidence}
    };
}

json advisory_to_json(const ailee::Advisory& adv) {
    return {
        {"posture", posture_report_to_json(adv.posture)},
        {"recommended_action", adv.recommended_action},
        {"action_approved", adv.action_approved}
    };
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "AILEE build id: " << AILEE_BUILD_ID << "\n";
    std::cout << "Rust prover hash: " << AILEE_RUST_PROVER_HASH << "\n";

    httplib::Server svr;

    ailee::AileeConfig config = ailee::load_config("ailee_config.json");
    ailee::AileeContext ctx = ailee::ailee_init(config);

    svr.Post("/evaluate_posture", [&ctx](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            auto snap = parse_snapshot(j);
            auto report = ailee::ailee_evaluate_posture(snap, ctx);
            res.set_content(posture_report_to_json(report).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/score_risk", [&ctx](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            auto snap = parse_snapshot(j);
            auto score = ailee::ailee_score_risk(snap, ctx);
            res.set_content(risk_score_to_json(score).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/advise_action", [&ctx](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            auto snap = parse_snapshot(j["snapshot"]);
            ailee::Policy policy;
            if (j.contains("policy")) {
                policy = parse_policy(j["policy"]);
            } else {
                policy = ailee::load_policy("policy.json");
            }
            auto adv = ailee::ailee_advise_action(snap, policy, ctx);
            res.set_content(advisory_to_json(adv).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::cout << "Starting AILEE Layer-2 Advisory Service on port " << port << "..." << std::endl;
    svr.listen("0.0.0.0", port);

    ailee::ailee_shutdown(ctx);
    return 0;
}
