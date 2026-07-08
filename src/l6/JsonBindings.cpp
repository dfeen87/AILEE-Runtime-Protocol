#include "l6/JsonBindings.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace ailee {
namespace l6 {

namespace {

// Helper to reliably format double avoiding varying decimal points on different platforms.
// We strictly use integer math and strings to avoid std::to_string floating-point issues,
// since Phase 16 specifies "no floating-point nondeterminism".
std::string format_double_stable(double val) {
    // Determine precision: we format as X.YY
    // Convert to uint64_t scaling by 100
    // The instructions say "coherence rounding" happens in FederationExport.
    // So we assume the double is already rounded to 2 decimal places logically,
    // but just to be safe, we round and convert here deterministically.

    // We treat coherence usually as a percentage or fraction.
    // Scale by 100 and round to nearest int, then format manually.
    // (e.g. 99.5 -> "99.50")
    int64_t scaled = static_cast<int64_t>(val * 100.0 + (val >= 0 ? 0.5 : -0.5));
    bool neg = scaled < 0;
    if (neg) scaled = -scaled;

    int64_t integer_part = scaled / 100;
    int64_t fractional_part = scaled % 100;

    std::string frac_str = std::to_string(fractional_part);
    if (frac_str.length() == 1) {
        frac_str = "0" + frac_str;
    }

    return (neg ? "-" : "") + std::to_string(integer_part) + "." + frac_str;
}

std::string escape_string(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (char c : str) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '\b') {
            out += "\\b";
        } else if (c == '\f') {
            out += "\\f";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (static_cast<unsigned char>(c) <= 0x1f) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
            out += buf;
        } else {
            out += c;
        }
    }
    return out;
}

// Minimalistic JSON parser helpers
void skip_whitespace(const std::string& json, size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) {
        pos++;
    }
}

void expect_char(const std::string& json, size_t& pos, char expected) {
    skip_whitespace(json, pos);
    if (pos >= json.size() || json[pos] != expected) {
        throw std::runtime_error(std::string("Expected '") + expected + "' at pos " + std::to_string(pos));
    }
    pos++;
}

std::string parse_string(const std::string& json, size_t& pos) {
    expect_char(json, pos, '"');
    std::string out;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\') {
            pos++;
            if (pos >= json.size()) throw std::runtime_error("Unexpected end of string escape");
            char c = json[pos++];
            if (c == '"') out += '"';
            else if (c == '\\') out += '\\';
            else if (c == 'b') out += '\b';
            else if (c == 'f') out += '\f';
            else if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else if (c == 'u') {
                if (pos + 4 > json.size()) throw std::runtime_error("Invalid unicode escape");
                pos += 4; // Simplified, not actually decoding unicode for this minimal parser
                out += "?";
            } else {
                throw std::runtime_error("Invalid escape char");
            }
        } else {
            out += json[pos++];
        }
    }
    expect_char(json, pos, '"');
    return out;
}

uint64_t parse_uint(const std::string& json, size_t& pos) {
    skip_whitespace(json, pos);
    size_t start = pos;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        pos++;
    }
    if (start == pos) throw std::runtime_error("Expected number");
    return std::stoull(json.substr(start, pos - start));
}

double parse_double(const std::string& json, size_t& pos) {
    skip_whitespace(json, pos);
    size_t start = pos;
    if (pos < json.size() && json[pos] == '-') pos++;
    while (pos < json.size() && ((json[pos] >= '0' && json[pos] <= '9') || json[pos] == '.')) {
        pos++;
    }
    if (start == pos) throw std::runtime_error("Expected double");
    return std::stod(json.substr(start, pos - start));
}

std::string parse_key(const std::string& json, size_t& pos) {
    std::string key = parse_string(json, pos);
    expect_char(json, pos, ':');
    return key;
}

} // anonymous namespace

std::string JsonBindings::to_json(const ExternalEnvelope& env) {
    // Keys must be lexicographically sorted: "id", "payload_hash", "timestamp"
    std::string out = "{";
    out += "\"id\":" + std::to_string(env.id) + ",";
    out += "\"payload_hash\":\"" + escape_string(env.payload_hash) + "\",";
    out += "\"timestamp\":" + std::to_string(env.timestamp);
    out += "}";
    return out;
}

std::string JsonBindings::to_json(const ExternalBitcoinClockState& clock) {
    // Keys sorted: "consensus_time", "height", "interval_seconds"
    std::string out = "{";
    out += "\"consensus_time\":" + format_double_stable(clock.consensus_time) + ",";
    out += "\"height\":" + std::to_string(clock.height) + ",";
    out += "\"interval_seconds\":" + format_double_stable(clock.interval_seconds);
    out += "}";
    return out;
}

std::string JsonBindings::to_json(const ExternalReplayEvent& event) {
    // Keys sorted: "block_hash", "height", "txid", "type"
    std::string out = "{";
    out += "\"block_hash\":\"" + escape_string(event.block_hash) + "\",";
    out += "\"height\":" + std::to_string(event.height) + ",";
    out += "\"txid\":\"" + escape_string(event.txid) + "\",";
    out += "\"type\":" + std::to_string(event.type);
    out += "}";
    return out;
}

std::string JsonBindings::to_json(const ExternalClusterView& view) {
    // Keys sorted: "clock", "coherence", "envelopes", "node_count", "replay_events"
    std::string out = "{";

    out += "\"clock\":" + to_json(view.clock) + ",";
    out += "\"coherence\":" + format_double_stable(view.coherence) + ",";

    out += "\"envelopes\":[";
    for (size_t i = 0; i < view.envelopes.size(); ++i) {
        out += to_json(view.envelopes[i]);
        if (i < view.envelopes.size() - 1) {
            out += ",";
        }
    }
    out += "],";

    out += "\"node_count\":" + std::to_string(view.node_count) + ",";

    out += "\"replay_events\":[";
    for (size_t i = 0; i < view.replay_events.size(); ++i) {
        out += to_json(view.replay_events[i]);
        if (i < view.replay_events.size() - 1) {
            out += ",";
        }
    }
    out += "]";

    out += "}";
    return out;
}

std::string JsonBindings::to_json(const ExternalReplayTick& tick) {
    // Keys sorted: "cluster_view", "scheduler_state_hash", "telemetry_json", "tick_index"
    std::string out = "{";
    out += "\"cluster_view\":" + to_json(tick.cluster_view) + ",";
    out += "\"scheduler_state_hash\":\"" + escape_string(tick.scheduler_state_hash) + "\",";
    // telemetry_json is a string containing json or a plain string
    out += "\"telemetry_json\":\"" + escape_string(tick.telemetry_json) + "\",";
    out += "\"tick_index\":" + std::to_string(tick.tick_index);
    out += "}";
    return out;
}

ExternalEnvelope JsonBindings::from_json_envelope(const std::string& json) {
    ExternalEnvelope env;
    size_t pos = 0;
    expect_char(json, pos, '{');
    while (pos < json.size()) {
        skip_whitespace(json, pos);
        if (json[pos] == '}') {
            pos++;
            break;
        }
        std::string key = parse_key(json, pos);
        if (key == "id") {
            env.id = parse_uint(json, pos);
        } else if (key == "payload_hash") {
            env.payload_hash = parse_string(json, pos);
        } else if (key == "timestamp") {
            env.timestamp = parse_uint(json, pos);
        } else {
            throw std::runtime_error("Unknown key in envelope: " + key);
        }

        skip_whitespace(json, pos);
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] != '}') {
            throw std::runtime_error("Expected ',' or '}'");
        }
    }
    return env;
}

ExternalBitcoinClockState JsonBindings::from_json_clock(const std::string& json) {
    ExternalBitcoinClockState clock;
    size_t pos = 0;
    expect_char(json, pos, '{');
    while (pos < json.size()) {
        skip_whitespace(json, pos);
        if (json[pos] == '}') {
            pos++;
            break;
        }
        std::string key = parse_key(json, pos);
        if (key == "consensus_time") {
            clock.consensus_time = parse_double(json, pos);
        } else if (key == "height") {
            clock.height = parse_uint(json, pos);
        } else if (key == "interval_seconds") {
            clock.interval_seconds = parse_double(json, pos);
        } else {
            throw std::runtime_error("Unknown key in clock state: " + key);
        }

        skip_whitespace(json, pos);
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] != '}') {
            throw std::runtime_error("Expected ',' or '}'");
        }
    }
    return clock;
}

ExternalReplayEvent JsonBindings::from_json_replay_event(const std::string& json) {
    ExternalReplayEvent event;
    size_t pos = 0;
    expect_char(json, pos, '{');
    while (pos < json.size()) {
        skip_whitespace(json, pos);
        if (json[pos] == '}') {
            pos++;
            break;
        }
        std::string key = parse_key(json, pos);
        if (key == "block_hash") {
            event.block_hash = parse_string(json, pos);
        } else if (key == "height") {
            event.height = parse_uint(json, pos);
        } else if (key == "txid") {
            event.txid = parse_string(json, pos);
        } else if (key == "type") {
            event.type = static_cast<uint8_t>(parse_uint(json, pos));
        } else {
            throw std::runtime_error("Unknown key in replay event: " + key);
        }

        skip_whitespace(json, pos);
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] != '}') {
            throw std::runtime_error("Expected ',' or '}'");
        }
    }
    return event;
}

ExternalClusterView JsonBindings::from_json_view(const std::string& json) {
    ExternalClusterView view;
    size_t pos = 0;
    expect_char(json, pos, '{');
    while (pos < json.size()) {
        skip_whitespace(json, pos);
        if (json[pos] == '}') {
            pos++;
            break;
        }
        std::string key = parse_key(json, pos);
        if (key == "coherence") {
            view.coherence = parse_double(json, pos);
        } else if (key == "node_count") {
            view.node_count = parse_uint(json, pos);
        } else if (key == "clock") {
            skip_whitespace(json, pos);
            size_t start_obj = pos;
            int depth = 0;
            while (pos < json.size()) {
                if (json[pos] == '{') depth++;
                else if (json[pos] == '}') depth--;
                pos++;
                if (depth == 0) break;
            }
            std::string clock_json = json.substr(start_obj, pos - start_obj);
            view.clock = from_json_clock(clock_json);
        } else if (key == "envelopes") {
            expect_char(json, pos, '[');
            while (pos < json.size()) {
                skip_whitespace(json, pos);
                if (json[pos] == ']') {
                    pos++;
                    break;
                }

                size_t start_obj = pos;
                int depth = 0;
                while (pos < json.size()) {
                    if (json[pos] == '{') depth++;
                    else if (json[pos] == '}') depth--;
                    pos++;
                    if (depth == 0) break;
                }
                std::string env_json = json.substr(start_obj, pos - start_obj);
                view.envelopes.push_back(from_json_envelope(env_json));

                skip_whitespace(json, pos);
                if (json[pos] == ',') {
                    pos++;
                } else if (json[pos] != ']') {
                    throw std::runtime_error("Expected ',' or ']' in envelopes array");
                }
            }
        } else if (key == "replay_events") {
            expect_char(json, pos, '[');
            while (pos < json.size()) {
                skip_whitespace(json, pos);
                if (json[pos] == ']') {
                    pos++;
                    break;
                }

                size_t start_obj = pos;
                int depth = 0;
                while (pos < json.size()) {
                    if (json[pos] == '{') depth++;
                    else if (json[pos] == '}') depth--;
                    pos++;
                    if (depth == 0) break;
                }
                std::string ev_json = json.substr(start_obj, pos - start_obj);
                view.replay_events.push_back(from_json_replay_event(ev_json));

                skip_whitespace(json, pos);
                if (json[pos] == ',') {
                    pos++;
                } else if (json[pos] != ']') {
                    throw std::runtime_error("Expected ',' or ']' in replay_events array");
                }
            }
        } else {
            throw std::runtime_error("Unknown key in cluster view: " + key);
        }

        skip_whitespace(json, pos);
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] != '}') {
            throw std::runtime_error("Expected ',' or '}'");
        }
    }
    return view;
}

ExternalReplayTick JsonBindings::from_json_tick(const std::string& json) {
    ExternalReplayTick tick;
    size_t pos = 0;
    expect_char(json, pos, '{');
    while (pos < json.size()) {
        skip_whitespace(json, pos);
        if (json[pos] == '}') {
            pos++;
            break;
        }
        std::string key = parse_key(json, pos);
        if (key == "tick_index") {
            tick.tick_index = parse_uint(json, pos);
        } else if (key == "scheduler_state_hash") {
            tick.scheduler_state_hash = parse_string(json, pos);
        } else if (key == "telemetry_json") {
            tick.telemetry_json = parse_string(json, pos);
        } else if (key == "cluster_view") {
            skip_whitespace(json, pos);
            size_t start_obj = pos;
            int depth = 0;
            while (pos < json.size()) {
                if (json[pos] == '{') depth++;
                else if (json[pos] == '}') depth--;
                pos++;
                if (depth == 0) break;
            }
            std::string view_json = json.substr(start_obj, pos - start_obj);
            tick.cluster_view = from_json_view(view_json);
        } else {
            throw std::runtime_error("Unknown key in replay tick: " + key);
        }

        skip_whitespace(json, pos);
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] != '}') {
            throw std::runtime_error("Expected ',' or '}'");
        }
    }
    return tick;
}

} // namespace l6
} // namespace ailee
