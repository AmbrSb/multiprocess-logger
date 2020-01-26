#pragma once

#include <boost/lockfree/queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>  
#include <boost/lockfree/policies.hpp>

#include "ring_common.h"


using ring_buffer = boost::lockfree::queue<elem,
                                           boost::lockfree::capacity<RING_CAPACITY>,
                                           boost::lockfree::fixed_sized<true>
                                           >;
