#include <thread>
// interpreter.cpp
#include "config_types.h"
#include "metrics.h"
#include "policies.h"
#include "config_hot_reload.h"
#include <unordered_map>
#include <deque>
#include <iostream>

struct Engine {
  // expose sampling sources
  double sample_latency_ms();
  double sample_tps();
  double sample_entropy();
  double sample_heat_w();
  void adjust_delay(int delta_ms);
  void switch_route(const std::string& route);
};

int main() {
  Engine engine;

  // Logging
  auto log = [](const std::string& m){ std::cout << "[interp] " << m << "\n"; };

  // Live state
  std::unordered_map<std::string, SignalData> signal_store;
  std::unordered_map<std::string, double> vars; // metrics + exposed values

  // Apply config atomically
  Config current;
  auto apply_cfg = [&](const Config& cfg){
    current = cfg;
    signal_store.clear();
    for (const auto& s : cfg.signals) {
      signal_store.emplace(s.name, SignalData{s.name, {}});
    }
    log("config applied: signals="+std::to_string(cfg.signals.size())+" metrics="+std::to_string(cfg.metrics.size()));
  };

  ConfigReloader reloader({.file="config.yaml", .fmt=ConfigFormat::YAML}, apply_cfg, log);

  // Policy runner actions
  ActionFns actions{
    .adjust_propagation_delay = [&](int d){ engine.adjust_delay(d); },
    .switch_route = [&](const std::string& r){ engine.switch_route(r); }
  };
  PolicyRunner policy_runner(current.policies, actions, log);

  size_t tick_ms = 50;
  for (;;) {
    reloader.tick();

    // 1) sample signals
    for (auto& [name, sig] : signal_store) {
      double v = 0.0;
      if (name=="latency_ms") v = engine.sample_latency_ms();
      else if (name=="tps") v = engine.sample_tps();
      else if (name=="entropy") v = engine.sample_entropy();
      else if (name=="heat_w") v = engine.sample_heat_w();
      sig.window.push_back(v);
      if (sig.window.size() > 2000) sig.window.pop_front(); // cap
      vars[name] = v; // raw exposure
    }

    // 2) compute metrics
    MetricContext mctx;
    mctx.step_ms = tick_ms;
    for (auto& [name, sig] : signal_store) mctx.signals[name] = &sig;

    for (const auto& m : current.metrics) {
      auto fn = MetricsRegistry::instance().get(m.type);
      double val = fn(mctx, m.signals);
      vars[m.name] = val;
    }

    // 3) policies
    policy_runner.step(vars);

    // 4) outputs (example CSV)
    // write_csv(vars); // implement buffered writer with fsync batching

    // 5) sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(tick_ms));
  }
  return 0;
}
