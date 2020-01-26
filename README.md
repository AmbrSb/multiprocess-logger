# Introduction
This library enables a scenario in which multiple process (probably running on different machines) want to send data (log) to a single collector process, very efficiently via shared memory.

The system has 3 modules:
1. *Spring*: client processes use this module to be able to generate/send data to extractors
2. *Extracotr*: The module that allows one to retrieve data produced by Spring clients.
3. *Registry*: A central registry that clients register their operational information with, so that extractors can look them up.

# Communication Mechanisms
## Shared Memory
This system uses Boost interprocess queues in shared memory segments shared between springs and extractors to maximize throughput. Each spring sets up its own shared SPSC queue to be read by an Extractor instance.
## gRPC
Spring and Extractors communicate with the Registry via gRPC/TCP. The frequency of this type of interaction in this system in minimal. So this should not have a noticable effect on the overall performance.

# Example
Using the spring can be as easy as:
```C++
#include <registry_client.hpp>
Spring sp{"process_34", "cp_chan", 128, sizeof(elem)};
sp.Push("[128572] a log item is here", 128570);
```
Here `python2.7` is the chosen name of the producer process, and `cp_chan` is the name of the sub-channel that can be looked up by an Extractor, and `128` is the size of the queue.

And as for the Extractor:
```C++
Extractor ex{"process_34", "cp_chan"};
elem* e = ex.Pop();
```
