#include <registry_client.hpp>
#include "spring_lcl.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <registry_client.hpp>

#include "spring_lcl.hpp"
#include <ring.h>

Spring::Spring(std::string ownr_name,
               std::string channel_name,
               std::size_t n,
               std::size_t sz,
               std::string addr,
               in_port_t port)
{
    using namespace registry;
    auto ring_name = ownr_name + "_" + channel_name;
    sockaddr_in reg_sin = {AF_INET, port, 0};
    inet_pton(AF_INET, addr.c_str(), &(reg_sin.sin_addr));
    SpringRegistryClient const src{ownr_name, RegistryLocation{reg_sin}};
    BufferLocation bloc = BufferLocation{channel_name};
    src.publish(bloc);
    ring_ = ring_init(ring_name.c_str(), n, sz);
}

Spring::~Spring()
{}

void
Spring::Push(std::string data, std::size_t id)
{
    elem e;
    e.id = id;
    snprintf(e.data, sizeof(e.data), "%s", data.c_str());
    ring_enqueue(ring_, &e);
}
