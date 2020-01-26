#pragma once

#include <tuple>
#include <string>
#include <exception>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <ring.h>


struct ChannelNotFound: public std::exception {};

/**
 * Used by client to lookup the registry information of any
 * Spring we are interested in and then actually reading from
 * their exposed ring buffers.
 * 
 */
class Extractor {
public:
    Extractor(std::string ownr_name, std::string channel_name,
              std::string addr = "127.0.0.1", in_port_t port = 40040);
    Extractor(Extractor const&) = delete;
    Extractor(Extractor&&) = delete;
    Extractor& operator=(Extractor const&) = delete;
    Extractor& operator=(Extractor&&) = delete;
    ~Extractor();

    elem* Pop();

private:
    /**
     * A multi-producer multi-consumer lockfree ring buffer
     * that resides in a shared memory by all interested parties.
     * This is unique for all BufferLocation instances that
     * compare equal.
     */
    ring* ring_;
};
