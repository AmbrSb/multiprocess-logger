#pragma once

#include <cstdint>
#include <string>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ring.h>

/**
 * Used by any client to create a Spring to register on a
 * Registry and generate data items and publish them to
 * a ring buffer.
 * 
 */
class Spring {
public:
    Spring(std::string ownr_name,
                std::string channel_name,
                std::size_t n,
                std::size_t sz,
                std::string addr = "127.0.0.1",
                in_port_t port = 40040);
    Spring(Spring const&) = delete;
    Spring(Spring&&) = delete;
    Spring& operator=(Spring const&) = delete;
    Spring& operator=(Spring&&) = delete;

    void
    Push(std::string data, std::size_t id = 0);

    ~Spring();

private:
    /**
     * A multi-producer multi-consumer lockfree ring buffer
     * that resides in a shared memory by all interested parties.
     * This is unique for all BufferLocation instances that
     * compare equal.
     */
    ring* ring_;
};