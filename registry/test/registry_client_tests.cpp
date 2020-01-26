#include <netinet/in.h>
#include <arpa/inet.h>

#include <gtest/gtest.h>
#include "registry_client.hpp"
#include "registry_common.hpp"
#include "../src/registry_core.hpp"


using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

namespace {

using registry::Registry;
using registry::SpringRegistryClient;
using registry::ExtractorRegistryClient;
using registry::BufferLocation;
using registry::RegistryLocation;
using registry::ServerComChannel;
using namespace std::literals;

TEST(SpringRegistryClient, publish_unpublish) {
    sockaddr_in reg_sin = {AF_INET, 40040, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    SpringRegistryClient const src{"client0"s, RegistryLocation{reg_sin}};
    BufferLocation bloc = BufferLocation{"ring_name2"};
    src.publish(bloc);
    src.unpublish("TestingSpring_01"s);
}

TEST(SpringRegistryClient, empty_bufferlocation_name) {
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    SpringRegistryClient const src{"client0"s, RegistryLocation{reg_sin}};
    BufferLocation bloc = BufferLocation{"ring_name2"};
    ASSERT_DEATH(BufferLocation{""}, "");
}

TEST(ExtractorRegistryClient, register_callback) {
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ExtractorRegistryClient erc{RegistryLocation{reg_sin}};
    BufferLocation bloc = BufferLocation{"ring_name2"};
    // XXX register_callback is not implemented yet.
    erc.register_callback("*"s, [](){});
}

class ClientServerTest1 : public ::testing::Test {
public:
    ClientServerTest1()
        : srv_ {sockaddr_in{AF_INET, 50051, 0}}
    {}

protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    Registry<ServerComChannel> srv_;
};

TEST_F(ClientServerTest1,  Register1WithSprngLookupWithExt) {
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));

    SpringRegistryClient const src{"client0"s, RegistryLocation{reg_sin}};
    BufferLocation bloc = BufferLocation{"ring_name1"};
    src.publish(bloc);

    ExtractorRegistryClient erc{RegistryLocation{reg_sin}};
    auto result = erc.Lookup("clientXYZ"s);
    ASSERT_EQ(result.size(), 0);

    result = erc.Lookup("client0"s);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].GetLocation().name, "ring_name1");
}


class ClientServerTest2 : public ::testing::Test {
public:
    ClientServerTest2()
        : srv_ {sockaddr_in{AF_INET, 50051, INADDR_ANY}}
    {}

protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    Registry<ServerComChannel> srv_;
};

TEST_F(ClientServerTest2,  Register1WithSprngRemove1LookupWithExt) {
    sockaddr_in reg_sin = {AF_INET, 50051, INADDR_ANY};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));

    SpringRegistryClient const src{"client0"s, RegistryLocation{reg_sin}};
    BufferLocation bloc = BufferLocation{"ring_nameX"};
    src.publish(bloc);
    bloc = BufferLocation{"ring_nameY"};
    src.publish(bloc);

    ExtractorRegistryClient erc{RegistryLocation{reg_sin}};
    auto result = erc.Lookup("client0"s);
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].GetLocation().name, "ring_nameX");
    ASSERT_EQ(result[1].GetLocation().name, "ring_nameY");

    bloc = BufferLocation{"ring_nameY"};
    src.unpublish(bloc);

    result = erc.Lookup("client0"s);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].GetLocation().name, "ring_nameX");
}

}
