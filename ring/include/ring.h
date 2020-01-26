#pragma once

#include <stddef.h>

#include "ring_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a Boost MPMC queue in shared memory segment
 * 
 * @param name The name of the queue.
 * @param n The capacity of the queue.
 * @param elemsz The size of individual items written to the queue.
 * @return struct ring* 
 */
struct ring* ring_init(char const* name, size_t n, size_t elemsz);

/**
 * @brief Attach a ring to a predefined Boost MPMC queue in
 * a shared memory segment.
 * 
 * @param name The name of the queue.
 * @return struct ring* 
 */
struct ring* ring_lookup(char const* name);

/**
 * @brief Destroy a predefined Boost MPMC queue in a shared memory
 * segment.
 * 
 * @param name The name of the queue.
 * @return struct ring* 
 */
int
ring_free(struct ring* r);

int
ring_enqueue(struct ring* r, struct elem* e);

int
ring_dequeue(struct ring* r, struct elem** e);

#ifdef __cplusplus
}
#endif
