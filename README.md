# RESMACK

## System Setup

```
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
# 
```

## Dependencies

Clone this repository with:

```
git clone PATH_TO_THIS_REPO
git submodule init
git submodule update
```

Additionally, the following must be installed on the system:

* libssl, libssl-dev (`openssh/sha.h`, links against `-lcrypto`)
* libunwind, libunwind-dev (usually already exists)

## Structure

Resmack is separated into separate sub-projects for each distinct piece of
functionality. Each project 

| project                | description                                                          |
|------------------------|----------------------------------------------------------------------|
| `libresmack`           | The core grammar functionality                                       |
| `libresmack_fuzz`      | Fuzzing-specific functionality (corpus, coverage, grammar mutation)  |
| `libresmack_fuzz_main` | Fuzzing-specific functionality (corpus, coverage, grammar mutation)  |
| `resmack-perf`         | A sample program to test performance of generating data with resmack |
| `resmack`              | UNFINISHED - The main CLI for resmack that uses `libresmack_fuzz`    |


## Architecture

The Tracer is more like a worker. It will continuously create/destroy child fuzzing processes
as-needed. Often the created child process can be used for many fuzzing iterations without
needing to be recreated.

Each evaluation loop, the `Tracer` works with a `ProcessLauncher` instance. The `ProcessLauncher`
instance defines the specifics of how a target should be launched. There is one virtual function
that ProcessLauncher subclasses must implement: `pid_t Spawn(...)`.

Currently the only process launcher is the `ForkLauncher` ProcessLauncher. This is where a snapshot-
based process launcher would be implemented.

A `Tracer` is intended to be `Start`ed once, and then `Join`ed. After it's started, the tracer
takes care of launching new fuzzing processes (as needed) through the `ProcessLauncher`. It creates
a separate thread to monitor the created child process (so the thread can set its own signal handlers)

Main()
  │
  └─ Tracer[n]
      Tracee
    ProcessLauncher
