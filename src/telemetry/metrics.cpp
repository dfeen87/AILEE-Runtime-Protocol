// metrics.cpp
#include "metrics.h"
#include <cmath>
#include <stdexcept>

MetricsRegistry& MetricsRegistry::instance(){ static MetricsRegistry reg; return reg; }
void MetricsRegistry::register_metric(const std::string& n, MetricFn f){ map_[n]=std::move(f); }
MetricFn MetricsRegistry::get(const std::string& n) const {
  auto it = map_.find(n);
  if (it == map_.end()) throw std::runtime_error("metric not found: "+n);
  return it->second;
}

static double pearson(const std::deque<double>& x, const std::deque<double>& y) {
  size_t n = std::min(x.size(), y.size());
  if (n < 2) return 0.0;
  double sx=0, sy=0, sxx=0, syy=0, sxy=0;
  for (size_t i=0;i<n;++i){ sx+=x[i]; sy+=y[i]; sxx+=x[i]*x[i]; syy+=y[i]*y[i]; sxy+=x[i]*y[i]; }
  double nd = static_cast<double>(n);
  double num = nd*sxy - sx*sy;
  double den = std::sqrt((nd*sxx - sx*sx)*(nd*syy - sy*sy));
  if (den <= 1e-12) return 0.0;
  double r = num/den;
  if (std::isnan(r) || std::isinf(r)) return 0.0;
  return std::max(-1.0, std::min(1.0, r));
}

// correlation_average: ordered pairs i != j
static double corr_avg_metric(const MetricContext& ctx, const std::vector<std::string>& names) {
  size_t N = names.size();
  if (N < 2) return 0.0;
  double sum = 0.0;
  size_t pairs = 0;
  try {
    for (size_t i=0;i<N;++i){
      auto xi = ctx.signals.at(names[i])->window;
      for (size_t j=0;j<N;++j){
        if (i==j) continue;
        auto xj = ctx.signals.at(names[j])->window;
        sum += pearson(xi,xj);
        ++pairs;
      }
    }
  } catch (const std::exception& ex) {
    return 0.0; // Signal not found
  }
  if (pairs == 0) return 0.0;
  return sum / static_cast<double>(pairs);
}

// Example EWMA metric
static double ewma_metric(const MetricContext& ctx, const std::vector<std::string>& names) {
  if (names.size() != 1) return 0.0;
  try {
    const auto& w = ctx.signals.at(names[0])->window;
    if (w.empty()) return 0.0;
    double alpha = 0.2; // tune or from config
    double s = w.front();
    for (size_t i=1;i<w.size();++i) s = alpha*w[i] + (1.0-alpha)*s;
    return s;
  } catch (const std::exception& ex) {
    return 0.0; // Signal not found
  }
}

// Register at startup
struct MetricsInit {
  MetricsInit(){
    MetricsRegistry::instance().register_metric("correlation_average", corr_avg_metric);
    MetricsRegistry::instance().register_metric("ewma", ewma_metric);
  }
} metrics_init;
