import sys

with open('src/l6/JsonBindings.cpp', 'r') as f:
    content = f.read()

to_json_env_end = content.find('std::string JsonBindings::to_json(const ExternalClusterView& view)')
if to_json_env_end == -1:
    sys.exit(1)

new_to_jsons = """std::string JsonBindings::to_json(const ExternalBitcoinClockState& clock) {
    // Keys sorted: "consensus_time", "height", "interval_seconds"
    std::string out = "{";
    out += "\\"consensus_time\\":" + format_double_stable(clock.consensus_time) + ",";
    out += "\\"height\\":" + std::to_string(clock.height) + ",";
    out += "\\"interval_seconds\\":" + format_double_stable(clock.interval_seconds);
    out += "}";
    return out;
}

std::string JsonBindings::to_json(const ExternalReplayEvent& event) {
    // Keys sorted: "block_hash", "height", "txid", "type"
    std::string out = "{";
    out += "\\"block_hash\\":\\"" + escape_string(event.block_hash) + "\\",";
    out += "\\"height\\":" + std::to_string(event.height) + ",";
    out += "\\"txid\\":\\"" + escape_string(event.txid) + "\\",";
    out += "\\"type\\":" + std::to_string(event.type);
    out += "}";
    return out;
}

"""

content = content[:to_json_env_end] + new_to_jsons + content[to_json_env_end:]

from_json_env_end = content.find('ExternalClusterView JsonBindings::from_json_view(const std::string& json)')
if from_json_env_end == -1:
    sys.exit(1)

new_from_jsons = """ExternalBitcoinClockState JsonBindings::from_json_clock(const std::string& json) {
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

"""

content = content[:from_json_env_end] + new_from_jsons + content[from_json_env_end:]

# Fix to_json view
view_to_json = """std::string JsonBindings::to_json(const ExternalClusterView& view) {
    // Keys sorted: "clock", "coherence", "envelopes", "node_count", "replay_events"
    std::string out = "{";

    out += "\\"clock\\":" + to_json(view.clock) + ",";
    out += "\\"coherence\\":" + format_double_stable(view.coherence) + ",";

    out += "\\"envelopes\\":[";
    for (size_t i = 0; i < view.envelopes.size(); ++i) {
        out += to_json(view.envelopes[i]);
        if (i < view.envelopes.size() - 1) {
            out += ",";
        }
    }
    out += "],";

    out += "\\"node_count\\":" + std::to_string(view.node_count) + ",";

    out += "\\"replay_events\\":[";
    for (size_t i = 0; i < view.replay_events.size(); ++i) {
        out += to_json(view.replay_events[i]);
        if (i < view.replay_events.size() - 1) {
            out += ",";
        }
    }
    out += "]";

    out += "}";
    return out;
}"""

import re
content = re.sub(r'std::string JsonBindings::to_json\(const ExternalClusterView& view\) \{.*?\n\}', view_to_json, content, flags=re.DOTALL)


with open('src/l6/JsonBindings.cpp', 'w') as f:
    f.write(content)
