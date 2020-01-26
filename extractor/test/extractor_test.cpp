#include <stdexcept>
#include <gtest/gtest.h>

#include <ring.h>
#include <spring.hpp>
#include <extractor.hpp>

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

TEST(Extractor, SpringPushExtractorPop) {
    std::size_t id = 987;
    std::string msg = "[XYZ] cool message";
    Spring sp{"Blinder", "chanx", 128, sizeof(elem)};
    sp.Push(msg, id);

    Extractor ex{"Blinder", "chanx"};
    elem* e = ex.Pop();
    ASSERT_EQ(e->id, id);
    ASSERT_STREQ(static_cast<char*>(e->data), msg.c_str());
}

void helper1() { Extractor ext{"ExtractingFromNonExistentChannel","chany"}; }

TEST(Extractor, ExtractingFromNonExistentChannel) {
    Spring sp{"ExtractingFromNonExistentChannel", "chanx", 128, sizeof(elem)};
    EXPECT_THROW(helper1(), ChannelNotFound);
}

void helper2() { Extractor ext{"BAD-ExtractingFromNonExistentOwner","chanx"}; }
TEST(Extractor, ExtractingFromNonExistentOwner) {
    Spring sp{"ExtractingFromNonExistentOwner", "chanx", 128, sizeof(elem)};
    EXPECT_THROW(helper2(), ChannelNotFound);
}

}

