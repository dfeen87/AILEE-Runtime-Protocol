import sys

with open('src/l6/JsonBindings.cpp', 'r') as f:
    content = f.read()

view_from_json_start = content.find('ExternalClusterView JsonBindings::from_json_view(const std::string& json)')
view_from_json_end = content.find('ExternalReplayTick JsonBindings::from_json_tick(const std::string& json)')

view_from_json = """ExternalClusterView JsonBindings::from_json_view(const std::string& json) {
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

"""

content = content[:view_from_json_start] + view_from_json + content[view_from_json_end:]

with open('src/l6/JsonBindings.cpp', 'w') as f:
    f.write(content)
