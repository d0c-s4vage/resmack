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
    * Process queues up changes for N iterations and then syncs with atomics
* Child process should never hold any locks
* Adding corpus entries can be slow
* Use unix domain socket to queue changes to corpus?
    * Hmm, this would be good. Keeps locks away from the children processes 
    * Would start putting things in place for a distributed setup

### Process Architecture

* Main process:
* Collects stats from each IPC

See the "vanilla" method of collecting counters here:
https://travisdowns.github.io/blog/2020/07/06/concurrency-costs.html

Process architecture:

   ┏━ parent ━━━━━━━━━━━━━━━━━━━━━┓           ┌───────┐
   ┃                              ┃   ┌──────>│ IPC 1 │
   ┃ ┌─ anon mmap Parent↔ Child ┳─────┘       └───────┘
   ┃ │ IPC Metadata             ┃ ┃               ^
   ┃ └──────────────────────────┻─────┐    ┌──────┘
   ┃ ┌─ named mmap All↔ All ────┓ ┃ ┌─│────┘  ┌───────┐
   ┃ │ Metadata                 ┣───┤ └──────>│ IPC 2 │
   ┃ └──────────────────────────┛ ┃ │         └───────┘
   ┃ ┌─ named mmap All↔ All ────┓ ┃ │             ^
   ┃ │ Corpus Items             ┣───┴─────────────┘
   ┃ └──────────────────────────┛ ┃
   ┃                              ┃
   ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

Metadata includes:

* Total Iterations
* Total runtime
* Total crashes
* Metadata for corpus items


## Creating MMAPS

1. Parent proc: create/load globally-shared mmaps
2. Create named mmaps for each process - to be deleted if already exist










