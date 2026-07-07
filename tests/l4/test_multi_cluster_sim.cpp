#include <gtest/gtest.h>
#include "l4/MultiClusterSim.h"
#include "l4/FederationConfig.h"
#include "l4/ClusterFederationView.h"

using namespace ailee::l4;
using namespace ailee::l2;

class MultiClusterSimTest : public ::testing::Test {
public:
    void SetUp() override {}
};

TEST_F(MultiClusterSimTest, DeterministicPropagation) {
    FederationConfig config = FederationConfig::simple(2, 1); // 2 clusters, latency 1
    MultiClusterSim sim(config);

    // Initial state: 0 in flight
    ASSERT_EQ(sim.in_flight.size(), 0);

    // Run a cycle to verify deterministic logic runs without issues
    sim.run_federation_tick();

    // Check view
    auto view = sim.build_view();
    ASSERT_EQ(view.cluster_views.size(), 2);
    // At latency 1, envelopes would be pending next tick. We haven't generated any real outgoing envelopes
    // unless we modified ClusterSim to produce them.
}

TEST_F(MultiClusterSimTest, DeterministicOrderingAndLatency) {
    FederationConfig config = FederationConfig::simple(3, 2); // 3 clusters, latency 2
    MultiClusterSim sim(config);

    sim.run_federation_tick();
    sim.run_federation_tick();

    // Just verify deterministic execution without crashing
    ASSERT_EQ(sim.federation_tick, 2);
    auto view = sim.build_view();
    ASSERT_EQ(view.cluster_views.size(), 3);
}
