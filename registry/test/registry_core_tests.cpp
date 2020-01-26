#include <gtest/gtest.h>
#include "registry_client.hpp"
#include "../src/registry_core.hpp"


using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

class FakeChan {
public:
    FakeChan() noexcept {}
};

namespace {

using namespace std::literals;

TEST(RegistryCore, RegisterWhenEmpty) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;
    registry::RegItem ri {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri);
    auto result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 1);
}

TEST(RegistryCore, RegisterTwoRemoveOneAndOne) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    reg.Register(ri1);
    reg.Register(ri2);
    auto result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 2);

    registry::RegItem ri3 {proc_name, shared_mem_prefix + "_01"s};
    reg.Unregister(ri3);
    result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 1);

    reg.Unregister(result[0]);
    result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 0);
}

TEST(RegistryCore, AddCallbackThenRegister) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, AddCallbackThenRegisterAndUnregisterAnother) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, AddCallbackThenRegisterUnregisterRegister) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    registry::RegItem ri3 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    reg.Unregister(ri3);
    ASSERT_EQ(recv, 0);
}

TEST(RegistryCore, RegisterThenAddCallback) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, RegisterAndUnregisterAnotherThenAddCallback) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, RegisterUnregisterRegisterThenAddCallback) {
    registry::Registry<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    registry::RegItem ri3 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    reg.Unregister(ri3);
    reg.AddCallback(shared_mem_prefix,
                    [&recv](auto const& items){ recv = std::size(items); });
    ASSERT_EQ(recv, 0);
}

/**
 *  ================ DB =================
 */

TEST(RegistryCore, RegisterDBWhenEmpty) {
    registry::RegistryDB<FakeChan> reg{};
    auto proc_name         = "host_process_01*"s;
    auto shared_mem_prefix = "/shared_mem"s;
    registry::RegItem ri {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri);
    auto result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 1);
}

TEST(RegistryCore, RegisterDBTwoRemoveOneAndOne) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    reg.Register(ri1);
    reg.Register(ri2);
    auto result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 2);

    registry::RegItem ri3 {proc_name, shared_mem_prefix + "_01"s};
    reg.Unregister(ri3);
    result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 1);

    reg.Unregister(result[0]);
    result = reg.Lookup(proc_name);
    ASSERT_EQ(std::size(result), 0);
}

TEST(RegistryCore, DBAddCallbackThenRegister) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, DBAddCallbackThenRegisterAndUnregisterAnother) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, DBAddCallbackThenRegisterUnregisterRegister) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    registry::RegItem ri3 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    reg.Unregister(ri3);
    ASSERT_EQ(recv, 0);
}

TEST(RegistryCore, DBRegisterThenAddCallback) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, DBRegisterAndUnregisterAnotherThenAddCallback) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    reg.AddCallback(proc_name,
                    [&recv](auto const& items){ recv = std::size(items); });
    ASSERT_EQ(recv, 1);
}

TEST(RegistryCore, DBRegisterUnregisterRegisterThenAddCallback) {
    registry::RegistryDB<FakeChan> reg;
    auto proc_name         = "host_process_01"s;
    auto shared_mem_prefix = "/shared_mem"s;

    int recv = -1;
    registry::RegItem ri1 {proc_name, shared_mem_prefix + "_01"s};
    registry::RegItem ri2 {proc_name, shared_mem_prefix + "_02"s};
    registry::RegItem ri3 {proc_name, shared_mem_prefix + "_01"s};
    reg.Register(ri1);
    reg.Unregister(ri2);
    reg.Unregister(ri3);
    reg.AddCallback(shared_mem_prefix,
                    [&recv](auto const& items){ recv = std::size(items); });
    ASSERT_EQ(recv, 0);
}

}
