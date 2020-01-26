#include <gtest/gtest.h>

#include <ring.h>
#include <spring.hpp>

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

#include <registry_client.hpp>

namespace {

using namespace std::literals;

TEST(Spring, Create) {
    Spring sp{"python2.7", "cp_chan", 128, sizeof(elem)};
    sp.Push("[128572] a log item is here", 128570);
}


}

