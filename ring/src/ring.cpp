#include "ring_lcl.hpp"

namespace bip = boost::interprocess;                                            

extern "C"
struct ring*
ring_init(char const* name, size_t n, size_t elemsz)
{
    if(n > RING_CAPACITY)
        return nullptr;

    char segname[SEGM_NAMESIZE];
    snprintf(segname, sizeof(segname), "SEG4xRING_%s", name);
    auto segment =
        new bip::managed_shared_memory(bip::open_or_create,
                                       segname,
                                       RING_CAPACITY * elemsz * 8 + (1024 * 1024));
    ring* r = (ring*) malloc(sizeof(*r));
    snprintf(r->name, sizeof(r->name), "%s", name);
    r->seg = static_cast<void*>(segment);
    r->queue = segment->find_or_construct<ring_buffer>(r->name)();
    assert(r->queue != nullptr);

    return r;
}

extern "C"
struct ring*
ring_lookup(char const* name)
{
    char segname[SEGM_NAMESIZE];
    snprintf(segname, sizeof(segname), "SEG4xRING_%s", name);
    auto segment =
        new bip::managed_shared_memory(bip::open_only, segname);
    ring* r = (ring*) malloc(sizeof(*r));
    snprintf(r->name, sizeof(r->name), "%s", name);
    r->seg = static_cast<void*>(segment);
    auto lookup_result = segment->find<ring_buffer>(r->name);
    r->queue = lookup_result.first;
    assert(r->queue != nullptr);

    return r;
}

extern "C"
int
ring_free(struct ring* r)
{
    auto seg = static_cast<bip::managed_shared_memory*>(r->seg);
    delete seg;
    free(r);
    return 0;
}
