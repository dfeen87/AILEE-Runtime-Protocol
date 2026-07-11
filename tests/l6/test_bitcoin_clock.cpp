#include <gtest/gtest.h>
#include "l6/BitcoinClock.h"

using namespace ailee::l6;

TEST(BitcoinClockTest, Instantiation) {
    BitcoinRpcConfig config{"http://127.0.0.1:8332", "rpcuser", "rpcpass"};
    BitcoinClock clock(config);
    // Since we don't have a real running local Bitcoin node, calling get_snapshot will throw,
    // so we just verify it can be instantiated without segfaults.
    EXPECT_TRUE(true);
}
