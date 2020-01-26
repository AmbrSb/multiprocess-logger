#include "extractor_lcl.hpp"

#include <iostream>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <registry_client.hpp>

Extractor::Extractor(std::string ownr_name, std::string channel_name,
                     std::string addr, in_port_t port)
{
    using namespace registry;
    sockaddr_in reg_sin = {AF_INET, port, 0};
    inet_pton(AF_INET, addr.c_str(), &(reg_sin.sin_addr));
    ExtractorRegistryClient erc{RegistryLocation{reg_sin}};
    auto result = erc.Lookup(ownr_name);
    if (result.size() == 0)
        throw ChannelNotFound{};
    bool found = false;
    for (auto const& itm: result) {
        if (itm.GetLocation().name == channel_name) {
            auto ring_name = ownr_name + "_" + channel_name;
            ring_ = ring_lookup(ring_name.c_str());
            found = true;
            break;
        }
    }
    if (!found)
        throw ChannelNotFound{};
    
}

elem*
Extractor::Pop()
{
    elem* e;
    auto ret = ring_dequeue(ring_, &e);
    if(0 == ret)
        return e;
    return nullptr;
}

Extractor::~Extractor()
{

}
