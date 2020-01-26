#pragma once

#include <stddef.h>
#include <inttypes.h>

#define SEGM_NAMESIZE       64
#define RING_NAMESIZE       64
#define RING_CAPACITY       (8 * 1024)

struct ring {
    char    name[RING_NAMESIZE];
    void*   seg;
    void*   queue;
};

size_t const kElemDataSz = 128;

struct elem {
    size_t      id;
    char      data[kElemDataSz];
};

