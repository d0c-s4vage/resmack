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
* cmake
* clang-20

## Example

The `test.cpp` and `run_on_grammar.cpp` files can be compiled with the resmack binary to produce
a simple fuzzer.

For example, if all dependencies are met, you should be able to do this:

```
# build the project and run the tests
cmake --workflow --preset full.release

# make sure we're using the just-built libraries
export LD_LIBRARY_PATH=build/release

# use the resmack binary to compile test.cpp and run_on_grammar.cpp into
# a fuzzer binary (a.out in your cwd)
build/release/resmack cc --asan -- test.cpp run_on_grammar.cpp -o a.out

# now run the fuzzer binary
./a.out -n $(nproc) --max-crashes 1
```

You can see the other CMake presets with:

```
cmake --workflow --list-presets
```

which should show:

```
$> cmake --workflow --list-presets
Available workflow presets:

  "build.debug"
  "build.debug.verbose"
  "build.release"
  "build.release.syms"
  "full.debug"
  "full.debug.verbose"
  "full.release"
  "full.release.syms"
```

## Structure

Resmack is separated into separate sub-projects for each distinct piece of
functionality. Each project 

| project                | description                                                          |
|------------------------|----------------------------------------------------------------------|
| `libresmack`           | The core grammar functionality                                       |
| `libresmack_fuzz`      | Fuzzing-specific functionality (corpus, coverage, grammar mutation)  |
| `libresmack_fuzz_main` | Fuzzing-specific functionality (corpus, coverage, grammar mutation)  |
| `resmack-perf`         | A sample program to test performance of generating data with resmack |
| `resmack`              | The main CLI for resmack that uses `libresmack_fuzz`                 |


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
