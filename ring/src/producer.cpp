#include "ring_lcl.hpp"

extern "C"
int
ring_enqueue(ring* r, elem* e)
{
    auto q = static_cast<ring_buffer*>(r->queue);
    if (q->push(*e))
        return 0;
    else
        return -1;
}
