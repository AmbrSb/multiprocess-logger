#include "ring_lcl.hpp"

extern "C"
int
ring_dequeue(ring* r, elem** e)
{
    auto q = static_cast<ring_buffer*>(r->queue);
    *e = (elem*)malloc(sizeof(**e));
    if(q->pop(**e)) {
        return 0;
    } else {
        free(*e);
        return -1;
    }
}
