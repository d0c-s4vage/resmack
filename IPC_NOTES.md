## Assumptions

* Counter increments will be frequent
* New corpus items are rare
* Maintaining list of sorted items can happen every `N` iterations
    * This can be performed by a separate thread
    * All "fuzzing" procs use this in a read-only manner
    * I don't think we even need locks for this

## MMAP Layout

Three parts: metadata (named), corpus data (named), and sorted items (anon)

* Metadata is:
    * Non-growing, global counters (e.g. time spent performing X)
    * Growing corpus metadata
* Corpus data is:
    * Growing, list with variable-sized entries
* Sorted items (anon)
    * At N intervals the items are sorted by the parent process

## Speed requirements

* Using corpus entries and counters must be *fast*
    * Process queues up changes after N iterations and then syncs with atomics
* Child process should never hold any locks
* Adding corpus entries can be slow
* Use unix domain socket to queue changes to corpus?
    * Hmm, this would be good. Keeps locks away from the children processes 
    * Would start putting things in place for a distributed setup

### Process Architecture

* Fuzzing processes communicate with the root process via domain sockets
    * Occasional updates are sent, every 0.1 seconds
    * Corpus metadata updates
    * Fuzzing iteration updates
    * New corpus items updates
* All fuzzing processes share the mmap metadata and corpus items in a read-only
    manner
    * No locking, period! Root process is the single writer to the mmap data
* Root process resorts the corpus items every X iterations
