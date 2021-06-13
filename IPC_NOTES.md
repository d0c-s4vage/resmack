## Assumptions

* Counter increments will be frequent
* New corpus items are rare
* Maintaining list of sorted items can happen every `N` iterations
    * This can be performed by a separate thread
    * All "fuzzing" procs use this in a read-only manner
    * I don't think we even need locks for this

## MMAP Layout

Two main parts: metadata (counters), and corpus data

* Counters are:
    * Non-growing, global counters (e.g. time spent performing X)
    * Growing corpus metadata
* Corpus data is:
    * Growing, list with variable-sized entries

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
* 















