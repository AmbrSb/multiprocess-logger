#include <iostream>
#include <utility>

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

using namespace std::literals;

using registry::Registry;
using registry::Filter;
using registry::ServerComChannel;
using registry::ClientComChannel;
using registry::RegistryLocation;

TEST(RegistryCom, RegisterLookupOneMatch) {
    in_addr addr;
    addr.s_addr = 0;
    sockaddr_in bind_sin = {AF_INET, 50051, addr};
    Registry<ServerComChannel> srv{bind_sin};

    addr.s_addr = 1;
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ClientComChannel clnt{RegistryLocation{reg_sin}};
    clnt.Register({std::string{"hellothere"}, std::string{"hellotherelocaton"}});
    Filter f{std::string{"hello"}};
    auto regs = clnt.Lookup(f);
    ASSERT_EQ(std::size(regs), 1);
}

TEST(RegistryCom, RegisterLookupNotMatch) {
    in_addr addr;
    addr.s_addr = 0;
    sockaddr_in bind_sin = {AF_INET, 50051, addr};
    Registry<ServerComChannel> srv{bind_sin};

    addr.s_addr = 1;
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ClientComChannel clnt{RegistryLocation{reg_sin}};
    clnt.Register({std::string{"hellothere"}, std::string{"hellotherelocaton"}});
    Filter f{std::string{"helxlo"}};
    auto regs = clnt.Lookup(f);
    ASSERT_EQ(std::size(regs), 0);
}

TEST(RegistryCom, RegisterLookupTwoMatches) {
    in_addr addr;
    addr.s_addr = 0;
    sockaddr_in bind_sin = {AF_INET, 50051, addr};
    Registry<ServerComChannel> srv{bind_sin};

    addr.s_addr = 1;
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ClientComChannel clnt{RegistryLocation{reg_sin}};
    clnt.Register({std::string{"hellothere"}, std::string{"hellotherelocaton"}});
    clnt.Register({std::string{"hellohere"}, std::string{"itsloc"}});
    Filter f{std::string{"hello"}};
    auto regs = clnt.Lookup(f);
    ASSERT_EQ(std::size(regs), 2);
}

TEST(RegistryCom, RegisterUnregisterLookupNotFind) {
    in_addr addr;
    addr.s_addr = 0;
    sockaddr_in bind_sin = {AF_INET, 50051, addr};
    Registry<ServerComChannel> srv{bind_sin};

    addr.s_addr = 1;
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ClientComChannel clnt{RegistryLocation{reg_sin}};
    clnt.Register({std::string{"hellothere"}, std::string{"hellotherelocaton"}});
    clnt.Unregister({std::string{"hellothere"}, std::string{"hellotherelocaton"}});
    Filter f{std::string{"hello"}};
    auto regs = clnt.Lookup(f);
    ASSERT_EQ(std::size(regs), 0);
}

TEST(RegistryCom, RegisterTwoUnregisterOneLookupFindOne) {
    in_addr addr;
    addr.s_addr = 0;
    sockaddr_in bind_sin = {AF_INET, 50051, addr};
    Registry<ServerComChannel> srv{bind_sin};

    addr.s_addr = 1;
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ClientComChannel clnt{RegistryLocation{reg_sin}};
    clnt.Register({std::string{"hellothere1"}, std::string{"hellotherelocaton1"}});
    clnt.Register({std::string{"hellothere2"}, std::string{"hellotherelocaton2"}});
    clnt.Unregister({std::string{"hellothere1"}, std::string{"hellotherelocaton1"}});
    Filter f{std::string{"hello"}};
    auto regs = clnt.Lookup(f);
    ASSERT_EQ(std::size(regs), 1);
}

TEST(RegistryCom, RegisterTwoUnregisterOneLookupFindZero) {
    in_addr addr;
    addr.s_addr = 0;
    sockaddr_in bind_sin = {AF_INET, 50051, addr};
    Registry<ServerComChannel> srv{bind_sin};

    addr.s_addr = 1;
    sockaddr_in reg_sin = {AF_INET, 50051, 0};
    inet_pton(AF_INET, "127.0.0.1", &(reg_sin.sin_addr));
    ClientComChannel clnt{RegistryLocation{reg_sin}};
    clnt.Register({std::string{"hellothere1"}, std::string{"hellotherelocaton1"}});
    clnt.Register({std::string{"anotherone"}, std::string{"hellotherelocaton2"}});
    clnt.Unregister({std::string{"hellothere1"}, std::string{"hellotherelocaton1"}});
    Filter f{std::string{"hello"}};
    auto regs = clnt.Lookup(f);
    ASSERT_EQ(std::size(regs), 0);
}

}
