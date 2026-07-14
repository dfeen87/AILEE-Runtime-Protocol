// config_loader.cpp
#include "config_loader.h"
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include <sstream>
#if __has_include(<toml++/toml.h>)
#include <toml++/toml.h>
#define AILEE_HAS_TOML 1
#endif
#if __has_include(<yaml-cpp/yaml.h>)
#include <yaml-cpp/yaml.h>
#define AILEE_HAS_YAML 1
#endif

static std::string slurp(const std::string& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss; ss << in.rdbuf();
  return ss.str();
}

static std::optional<Config> parse_yaml(const std::string& text) {
#if !defined(AILEE_HAS_YAML)
  (void)text;
  return std::nullopt;
#else
  try {
    YAML::Node root = YAML::Load(text);
    Config cfg;

    if (root["version"]) cfg.version = root["version"].as<int>();
    if (root["mode"]) cfg.mode = root["mode"].as<std::string>();
    if (root["step_ms"]) cfg.step_ms = root["step_ms"].as<size_t>();
    if (root["horizon_s"]) cfg.horizon_s = root["horizon_s"].as<size_t>();
    if (root["block_interval_ms"]) cfg.block_interval_ms = root["block_interval_ms"].as<size_t>();
    if (root["commitment_interval"]) cfg.commitment_interval = root["commitment_interval"].as<size_t>();

    if (root["signals"]) {
      const auto& signals_node = root["signals"];
      for (size_t idx = 0, n = signals_node.size(); idx < n; ++idx) {
        const auto& sig = signals_node[idx];
        SignalSpec s;
        if (!sig["name"]) { std::cerr << "YAML: signals[" << idx << "] missing 'name'\n"; return std::nullopt; }
        if (!sig["source"]) { std::cerr << "YAML: signals[" << idx << "] missing 'source'\n"; return std::nullopt; }
        if (!sig["window_ms"]) { std::cerr << "YAML: signals[" << idx << "] missing 'window_ms'\n"; return std::nullopt; }
        s.name = sig["name"].as<std::string>();
        s.source = sig["source"].as<std::string>();
        s.window_ms = sig["window_ms"].as<size_t>();
        cfg.signals.push_back(s);
      }
    }

    if (root["metrics"]) {
      const auto& metrics_node = root["metrics"];
      for (size_t idx = 0, n = metrics_node.size(); idx < n; ++idx) {
        const auto& met = metrics_node[idx];
        MetricSpec m;
        if (!met["name"]) { std::cerr << "YAML: metrics[" << idx << "] missing 'name'\n"; return std::nullopt; }
        if (!met["window_ms"]) { std::cerr << "YAML: metrics[" << idx << "] missing 'window_ms'\n"; return std::nullopt; }
        if (!met["stride_ms"]) { std::cerr << "YAML: metrics[" << idx << "] missing 'stride_ms'\n"; return std::nullopt; }
        m.name = met["name"].as<std::string>();
        m.type = met["type"] ? met["type"].as<std::string>() : "";
        if (met["signals"]) {
          for (const auto& sig : met["signals"]) {
            m.signals.push_back(sig.as<std::string>());
          }
        }
        m.window_ms = met["window_ms"].as<size_t>();
        m.stride_ms = met["stride_ms"].as<size_t>();
        cfg.metrics.push_back(m);
      }
    }

    if (root["policies"]) {
      const auto& policies_node = root["policies"];
      for (size_t idx = 0, n = policies_node.size(); idx < n; ++idx) {
        const auto& pol = policies_node[idx];
        PolicySpec p;
        if (!pol["name"]) { std::cerr << "YAML: policies[" << idx << "] missing 'name'\n"; return std::nullopt; }
        if (!pol["when"]) { std::cerr << "YAML: policies[" << idx << "] missing 'when'\n"; return std::nullopt; }
        p.name = pol["name"].as<std::string>();
        p.when = pol["when"].as<std::string>();
        if (pol["actions"]) {
          const auto& actions_node = pol["actions"];
          for (size_t aidx = 0, an = actions_node.size(); aidx < an; ++aidx) {
            const auto& act = actions_node[aidx];
            PolicyAction a;
            if (!act["type"]) { std::cerr << "YAML: policies[" << idx << "].actions[" << aidx << "] missing 'type'\n"; return std::nullopt; }
            a.type = act["type"].as<std::string>();
            if (act["args"]) {
              for (const auto& arg : act["args"]) {
                a.args[arg.first.as<std::string>()] = arg.second.as<std::string>();
              }
            }
            p.actions.push_back(a);
          }
        }
        cfg.policies.push_back(p);
      }
    }

    if (root["pipelines"]) {
      const auto& pipelines_node = root["pipelines"];
      for (size_t idx = 0, n = pipelines_node.size(); idx < n; ++idx) {
        const auto& pipe = pipelines_node[idx];
        PipelineSpec ps;
        if (!pipe["name"]) { std::cerr << "YAML: pipelines[" << idx << "] missing 'name'\n"; return std::nullopt; }
        if (!pipe["enabled"]) { std::cerr << "YAML: pipelines[" << idx << "] missing 'enabled'\n"; return std::nullopt; }
        ps.name = pipe["name"].as<std::string>();
        ps.enabled = pipe["enabled"].as<bool>();
        cfg.pipelines.push_back(ps);
      }
    }

    if (root["outputs"]) {
      const auto& outputs_node = root["outputs"];
      for (size_t idx = 0, n = outputs_node.size(); idx < n; ++idx) {
        const auto& out = outputs_node[idx];
        OutputSpec o;
        if (!out["type"]) { std::cerr << "YAML: outputs[" << idx << "] missing 'type'\n"; return std::nullopt; }
        if (!out["path"]) { std::cerr << "YAML: outputs[" << idx << "] missing 'path'\n"; return std::nullopt; }
        o.type = out["type"].as<std::string>();
        o.path = out["path"].as<std::string>();
        if (out["fields"]) {
          for (const auto& field : out["fields"]) {
            o.fields.push_back(field.as<std::string>());
          }
        }
        cfg.outputs.push_back(o);
      }
    }

    return cfg;
  } catch (const YAML::Exception& e) {
    std::cerr << "YAML parse error: " << e.what() << std::endl;
    return std::nullopt;
  } catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << std::endl;
    return std::nullopt;
  }
#endif
}

static std::optional<Config> parse_json(const std::string& text) {
  try {
    auto root = nlohmann::json::parse(text);
    Config cfg;

    if (root.contains("version")) cfg.version = root["version"].get<int>();
    if (root.contains("mode")) cfg.mode = root["mode"].get<std::string>();
    if (root.contains("step_ms")) cfg.step_ms = root["step_ms"].get<size_t>();
    if (root.contains("horizon_s")) cfg.horizon_s = root["horizon_s"].get<size_t>();

    if (root.contains("signals")) {
      for (const auto& sig : root["signals"]) {
        SignalSpec s;
        s.name = sig.value("name", "");
        s.source = sig.value("source", "");
        if (sig.contains("window_ms")) {
          s.window_ms = sig["window_ms"].get<size_t>();
        }
        cfg.signals.push_back(std::move(s));
      }
    }

    if (root.contains("metrics")) {
      for (const auto& met : root["metrics"]) {
        MetricSpec m;
        m.name = met.value("name", "");
        m.type = met.value("type", "");
        if (met.contains("signals")) {
          for (const auto& sig : met["signals"]) {
            m.signals.push_back(sig.get<std::string>());
          }
        }
        if (met.contains("window_ms")) {
          m.window_ms = met["window_ms"].get<size_t>();
        }
        if (met.contains("stride_ms")) {
          m.stride_ms = met["stride_ms"].get<size_t>();
        }
        cfg.metrics.push_back(std::move(m));
      }
    }

    if (root.contains("policies")) {
      for (const auto& pol : root["policies"]) {
        PolicySpec p;
        p.name = pol.value("name", "");
        p.when = pol.value("when", "");
        if (pol.contains("actions")) {
          for (const auto& act : pol["actions"]) {
            PolicyAction a;
            a.type = act.value("type", "");
            if (act.contains("args") && act["args"].is_object()) {
              for (auto it = act["args"].begin(); it != act["args"].end(); ++it) {
                if (it.value().is_string()) {
                  a.args[it.key()] = it.value().get<std::string>();
                } else {
                  a.args[it.key()] = it.value().dump();
                }
              }
            }
            p.actions.push_back(std::move(a));
          }
        }
        cfg.policies.push_back(std::move(p));
      }
    }

    if (root.contains("pipelines")) {
      for (const auto& pipe : root["pipelines"]) {
        PipelineSpec ps;
        ps.name = pipe.value("name", "");
        ps.enabled = pipe.value("enabled", false);
        cfg.pipelines.push_back(std::move(ps));
      }
    }

    if (root.contains("outputs")) {
      for (const auto& out : root["outputs"]) {
        OutputSpec o;
        o.type = out.value("type", "");
        o.path = out.value("path", "");
        if (out.contains("fields")) {
          for (const auto& field : out["fields"]) {
            o.fields.push_back(field.get<std::string>());
          }
        }
        cfg.outputs.push_back(std::move(o));
      }
    }

    return cfg;
  } catch (const nlohmann::json::exception& e) {
    std::cerr << "JSON parse error: " << e.what() << std::endl;
    return std::nullopt;
  } catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << std::endl;
    return std::nullopt;
  }
}

static std::optional<Config> parse_toml(const std::string& text) {
#if !defined(AILEE_HAS_TOML)
  (void)text;
  return std::nullopt;
#else
  try {
    auto root = toml::parse(text);
    Config cfg;

    if (auto v = root["version"].value<int>()) cfg.version = *v;
    if (auto v = root["mode"].value<std::string>()) cfg.mode = *v;
    if (auto v = root["step_ms"].value<int64_t>()) cfg.step_ms = static_cast<size_t>(*v);
    if (auto v = root["horizon_s"].value<int64_t>()) cfg.horizon_s = static_cast<size_t>(*v);

    if (auto signals = root["signals"].as_array()) {
      for (const auto& sigNode : *signals) {
        const auto* sig = sigNode.as_table();
        if (!sig) continue;
        SignalSpec s;
        if (auto v = sig->get_as<std::string>("name")) s.name = v->get();
        if (auto v = sig->get_as<std::string>("source")) s.source = v->get();
        if (auto v = sig->get_as<int64_t>("window_ms")) s.window_ms = static_cast<size_t>(v->get());
        cfg.signals.push_back(std::move(s));
      }
    }

    if (auto metrics = root["metrics"].as_array()) {
      for (const auto& metNode : *metrics) {
        const auto* met = metNode.as_table();
        if (!met) continue;
        MetricSpec m;
        if (auto v = met->get_as<std::string>("name")) m.name = v->get();
        if (auto v = met->get_as<std::string>("type")) m.type = v->get();
        if (auto v = met->get_as<int64_t>("window_ms")) m.window_ms = static_cast<size_t>(v->get());
        if (auto v = met->get_as<int64_t>("stride_ms")) m.stride_ms = static_cast<size_t>(v->get());
        if (auto sigs = met->get("signals")) {
          if (auto arr = sigs->as_array()) {
            for (const auto& sig : *arr) {
              if (auto v = sig.value<std::string>()) {
                m.signals.push_back(*v);
              }
            }
          }
        }
        cfg.metrics.push_back(std::move(m));
      }
    }

    if (auto policies = root["policies"].as_array()) {
      for (const auto& polNode : *policies) {
        const auto* pol = polNode.as_table();
        if (!pol) continue;
        PolicySpec p;
        if (auto v = pol->get_as<std::string>("name")) p.name = v->get();
        if (auto v = pol->get_as<std::string>("when")) p.when = v->get();

        if (auto actionsNode = pol->get("actions")) {
          if (auto actions = actionsNode->as_array()) {
            for (const auto& actNode : *actions) {
              const auto* act = actNode.as_table();
              if (!act) continue;
              PolicyAction a;
              if (auto v = act->get_as<std::string>("type")) a.type = v->get();
              if (auto argsNode = act->get("args")) {
                if (auto args = argsNode->as_table()) {
                  for (const auto& [key, value] : *args) {
                    if (auto v = value.value<std::string>()) {
                      a.args[key.str()] = *v;
                    } else {
                      a.args[key.str()] = toml::format(value);
                    }
                  }
                }
              }
              p.actions.push_back(std::move(a));
            }
          }
        }

        cfg.policies.push_back(std::move(p));
      }
    }

    if (auto pipelines = root["pipelines"].as_array()) {
      for (const auto& pipeNode : *pipelines) {
        const auto* pipe = pipeNode.as_table();
        if (!pipe) continue;
        PipelineSpec ps;
        if (auto v = pipe->get_as<std::string>("name")) ps.name = v->get();
        if (auto v = pipe->get_as<bool>("enabled")) ps.enabled = v->get();
        cfg.pipelines.push_back(std::move(ps));
      }
    }

    if (auto outputs = root["outputs"].as_array()) {
      for (const auto& outNode : *outputs) {
        const auto* out = outNode.as_table();
        if (!out) continue;
        OutputSpec o;
        if (auto v = out->get_as<std::string>("type")) o.type = v->get();
        if (auto v = out->get_as<std::string>("path")) o.path = v->get();
        if (auto fieldsNode = out->get("fields")) {
          if (auto fields = fieldsNode->as_array()) {
            for (const auto& field : *fields) {
              if (auto v = field.value<std::string>()) {
                o.fields.push_back(*v);
              }
            }
          }
        }
        cfg.outputs.push_back(std::move(o));
      }
    }

    return cfg;
  } catch (const toml::parse_error& e) {
    std::cerr << "TOML parse error: " << e.description() << std::endl;
    return std::nullopt;
  } catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << std::endl;
    return std::nullopt;
  }
#endif
}

ConfigResult load_config(const std::string& file, ConfigFormat fmt) {
  ConfigResult r;
  auto text = slurp(file);
  if (text.empty()) {
    r.errors.push_back({"Config file not found or empty", file});
    return r;
  }
  r.raw_text = text;
  std::optional<Config> parsed;
  switch (fmt) {
    case ConfigFormat::YAML: parsed = parse_yaml(text); break;
    case ConfigFormat::JSON: parsed = parse_json(text); break;
    case ConfigFormat::TOML: parsed = parse_toml(text); break;
  }
  if (!parsed) {
    r.errors.push_back({"Parse failed", file});
    return r;
  }
  std::vector<ConfigError> errs;
  if (!validate(*parsed, errs)) {
    r.errors = std::move(errs);
    return r;
  }
  r.cfg = std::move(parsed);
  return r;
}

bool validate(const Config& cfg, std::vector<ConfigError>& e) {
  auto add = [&](const std::string& m, const std::string& p) { e.push_back({m,p}); };
  if (cfg.mode != "simulation" && cfg.mode != "live") add("mode must be 'simulation' or 'live'", "mode");
  if (cfg.step_ms < 5 || cfg.step_ms > 1000) add("step_ms out of bounds [5..1000]", "step_ms");
  if (cfg.horizon_s < 10 || cfg.horizon_s > 86400) add("horizon_s out of bounds [10..86400]", "horizon_s");

  if (cfg.signals.empty()) add("at least one signal required", "signals");
  for (size_t i=0;i<cfg.signals.size();++i) {
    const auto& s = cfg.signals[i];
    if (s.name.empty()) add("signal.name required", "signals["+std::to_string(i)+"].name");
    if (s.source.empty()) add("signal.source required", "signals["+std::to_string(i)+"].source");
    if (s.window_ms < cfg.step_ms) add("signal.window_ms must be >= step_ms", "signals["+std::to_string(i)+"].window_ms");
  }

  for (size_t i=0;i<cfg.metrics.size();++i) {
    const auto& m = cfg.metrics[i];
    if (m.name.empty()) add("metric.name required", "metrics["+std::to_string(i)+"]");
    if (m.window_ms < cfg.step_ms) add("metric.window_ms >= step_ms", "metrics["+std::to_string(i)+"].window_ms");
    if (m.stride_ms < cfg.step_ms) add("metric.stride_ms >= step_ms", "metrics["+std::to_string(i)+"].stride_ms");
    if (m.signals.size() < 2) add("metric must reference >=2 signals", "metrics["+std::to_string(i)+"].signals");
  }

  for (size_t i=0;i<cfg.policies.size();++i) {
    const auto& p = cfg.policies[i];
    if (p.name.empty()) add("policy.name required", "policies["+std::to_string(i)+"]");
    if (p.when.empty()) add("policy.when expression required", "policies["+std::to_string(i)+"].when");
    if (p.actions.empty()) add("policy must have actions", "policies["+std::to_string(i)+"].actions");
  }

  for (size_t i=0;i<cfg.outputs.size();++i) {
    const auto& o = cfg.outputs[i];
    if (o.type != "csv") add("outputs.type currently supports 'csv' only", "outputs["+std::to_string(i)+"].type");
    if (o.path.empty()) add("outputs.path required", "outputs["+std::to_string(i)+"].path");
  }
  return e.empty();
}
