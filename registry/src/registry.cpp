#include <netinet/in.h>
#include <arpa/inet.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "registry_common.hpp"
#include "registry_core.hpp"

using namespace registry;
using namespace google;

namespace {

/**
 * Starts and blocks on a Registry service instance.
 * Returns when the service has stopped.
 * 
 * @param bind_sin The address the service should bind and
 * listen to.
 */
class ManagedService {
public:
    using ResultType = void;
    ManagedService() {}
    ManagedService(sockaddr_in bind_sin)
        : bind_sin_{bind_sin} {}
    void Run() {
        Registry<ServerComChannel> reg{bind_sin_};
        reg.Wait();
     }

private:
    sockaddr_in bind_sin_;
};

static bool port_validator(char const* flag, uint32 port)
{
    return (((port << 16) >> 16) == port);
}

DEFINE_string(ip, "0.0.0.0", "Bind address for Registry");
DEFINE_uint32(port, 40040, "Bind port for Registry");
DEFINE_validator(port, &port_validator);

in_addr
parse_ip(char const* addrstr)
{
    in_addr addr;
    int err = inet_pton(AF_INET, addrstr, &addr);
    if (err <= 0) {
        if (err == 0)
            LOG(ERROR) << "Invalid IP address";
        else
            PLOG(ERROR) << "inet_pton";
        exit(EXIT_FAILURE);
    }
    return addr;
}

sockaddr_in
parse_args(int argc, char* argv[])
{
    gflags::SetVersionString("1.0.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    in_addr addr = parse_ip(FLAGS_ip.c_str());
    in_port_t prt = FLAGS_port;
    sockaddr_in bind_sin = {AF_INET, prt, addr};

    return bind_sin;
}

}

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;

    sockaddr_in bind_sin = parse_args(argc, argv);
    ManagedService ms{bind_sin};
    ms.Run();

    google::ShutdownGoogleLogging();

    return 0;
}
