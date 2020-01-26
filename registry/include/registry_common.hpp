#pragma once

#include <variant>
#include <string>
#include <cassert>
#include <exception>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <gsl/pointers>

namespace registry
{

/**
 * @brief Possible address types for a Registry location and
 * also ring buffer location.
 * 
 */
using NetAddr = std::variant<std::monostate,
                             sockaddr_in,
                             sockaddr_in6>;

struct BufferLocation
{
    using NameType = std::string;

    /**
     * @brief Supported types of ring buffers.
     * 
     */
    enum Region {
        /// The buffer resides on the system computer as the
        /// client.
        kNear,
        /// The buffer resides on a remote system and should
        /// be accessed over a network.
        kFar
    };

    /// The name of this ring buffer that is being expoded.
    NameType name;
    /// The Region of this buffer location with respect
    /// to a given Registry.
    Region region;
    /// The NetAddr describing the location of this
    /// BufferLocation with respect to the Registry.
    NetAddr addr;

    BufferLocation(NameType n) noexcept
    : name{n},
      region {kNear} {
        assert(std::size(n) > 1);
    }

    BufferLocation(NameType n, NetAddr const& addr) noexcept
    : name{n},
      region {kFar},
      addr{addr} {
        assert(std::size(n) > 1);
    }

    BufferLocation& operator=(BufferLocation const& b) = default;
    BufferLocation& operator=(BufferLocation&& b) noexcept = default;
    BufferLocation(BufferLocation const& b) = default;
    BufferLocation(BufferLocation&& b) noexcept = default;
};

struct RegistryLocation: public NetAddr {

    RegistryLocation(): NetAddr{} {}
    RegistryLocation(sockaddr_in const& sin)
        : NetAddr{sin} {}
    RegistryLocation(sockaddr_in6 const& sin)
        : NetAddr{sin} {}
    RegistryLocation(RegistryLocation const&) noexcept = default;
    RegistryLocation(RegistryLocation &&) noexcept = default;

    operator std::string() const {
        char ipstr[128];
        char addrstr[160];
        if (std::holds_alternative<sockaddr_in>(*this)) {
            auto a = std::get<1>(*this);
            inet_ntop(AF_INET, &a.sin_addr, ipstr, sizeof(ipstr));
            snprintf(addrstr, sizeof(addrstr), "%s:%hu", ipstr, a.sin_port);
        } else if (std::holds_alternative<sockaddr_in6>(*this)) {
            auto a = std::get<2>(*this);
            inet_ntop(AF_INET6, &a.sin6_addr, ipstr, sizeof(ipstr));
            snprintf(addrstr, sizeof(addrstr), "%s:%hu", ipstr, a.sin6_port);
        }
        return std::string{addrstr};
    }
};

/**
 * @brief Represents individual registrations in a Registrt.
 * 
 */
class RegItem {
    public:
        RegItem(std::string const& n, BufferLocation l)
            : name_{n}, loc_{l} {
                assert(std::size(name_) > 0);
                assert(std::size(l.name) > 1);
        }
        template <typename T>
        explicit RegItem(T const& rgitm)
            : RegItem{rgitm.name(), rgitm.location()} {}
        RegItem(RegItem const& ri) = default;
        RegItem(RegItem&& ri) = default;
        RegItem& operator=(RegItem const& other) {
            this->name_ = other.name_;
            this->loc_ = other.loc_;
            return *this;
        }
        RegItem& operator=(RegItem && other) {
            this->name_ = std::move(other.name_);
            this->loc_ = std::move(other.loc_);
            return *this;
        }

        std::string const& GetName() const { return name_; }
        BufferLocation const& GetLocation() const { return loc_; }

        friend bool
        operator==(RegItem const&, RegItem const&);

    private:
        /**
         * This should generally describe the process that is
         * using the Spring. It is the same for all Springs
         * created by the process.
         */
        std::string name_;
        /** 
         * This describes a specific Spring in the process denoted
         * by name_. It has to be unique this process.
         */
        BufferLocation loc_;
};

inline bool
operator==(RegItem const& a, RegItem const& b)
{
    return a.name_     == b.name_ &&
           a.loc_.name == b.loc_.name;
}

/**
 * @brief Thrown when the gRPC stub returns any error status
 * in responce to a Lookup request.
 * 
 */
class LookupFailed final
      : public std::exception {};

/**
 * @brief This class is used to search a Registry for RegItems
 * that match a specific pattern.
 * 
 */
class Filter {
    private:
        using NameType = BufferLocation::NameType;

    public:
        Filter(NameType const& filter_text)
            : filter_{filter_text} {}
        template <typename T>
        explicit Filter(T const& fltr)
            : Filter{fltr.definition()} {}
        ~Filter() noexcept = default;

        bool operator() (NameType const& name) const {
            return name.find(filter_) != std::string::npos;
        }

        friend bool operator==(Filter const&, Filter const&);

        NameType const& GetFilterText() const { return filter_; }

    private:
        /// This is the specific pattern used to query a Registry
        BufferLocation::NameType filter_;
};

inline bool
operator==(Filter const& f1, Filter const& f2)
{
    return f1.filter_ == f2.filter_;
}

}
