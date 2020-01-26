#include <iostream>

#include <glog/logging.h>

#include "registry_core.hpp"
#include "registry_client.hpp"
#include "registry_lcl.hpp"

namespace registry {

class SpringRegistryClient::Impl
{
public:
    Impl(std::string const& n, RegistryLocation const& l);
    Impl(Impl const&) = delete;
    Impl(Impl &&) = delete;
    Impl& operator=(Impl const&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl() noexcept = default;

    void publish(BufferLocation const& location) const;
    void unpublish(BufferLocation const& name) const;

private:
    std::string const name_;
    ClientComChannel const clnt_;
};

SpringRegistryClient::Impl::Impl(std::string const& name, RegistryLocation const& l)
    : name_{name},
      clnt_{l}
{}

void
SpringRegistryClient::Impl::publish(BufferLocation const& location) const
{
    clnt_.Register(RegItem{name_, location});
}

void
SpringRegistryClient::Impl::unpublish(BufferLocation const& location) const
{
    clnt_.Unregister(RegItem{name_, location});
}

class ExtractorRegistryClient::Impl
{
public:
    Impl(RegistryLocation const& l);
    Impl(Impl const&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl const&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl() noexcept = default;

    std::vector<RegItem> Lookup(Filter const&) const;
    void register_callback(Filter const flt,
                           std::function<void()> cb);

private:
    ClientComChannel const clnt_;
};

ExtractorRegistryClient::Impl::Impl(RegistryLocation const& l)
    : clnt_{l}
{}

std::vector<RegItem>
ExtractorRegistryClient::Impl::Lookup(Filter const& fltr) const
{
    return clnt_.Lookup(fltr);
}

SpringRegistryClient::SpringRegistryClient(std::string const& name,
                                           RegistryLocation const& loc) noexcept
    : pimpl_{new Impl{name, loc}}
{}

void
SpringRegistryClient::publish(BufferLocation const& location) const
{
    pimpl_->publish(location);
}

void
SpringRegistryClient::unpublish(BufferLocation const& location) const
{
    pimpl_->unpublish(location);
}

SpringRegistryClient::~SpringRegistryClient() noexcept = default;

std::vector<RegItem>
ExtractorRegistryClient::Lookup(Filter const& fltr) const
{
    std::vector<RegItem> buffers;
    return pimpl_->Lookup(fltr);
}

void
ExtractorRegistryClient::register_callback(Filter const flt,
                                           std::function<void()> cb)
{

}

ExtractorRegistryClient::ExtractorRegistryClient(RegistryLocation const& loc) noexcept
    : pimpl_{new Impl{loc}}
{
    
}

ExtractorRegistryClient::~ExtractorRegistryClient() noexcept = default;

}
