# RESMACK

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

### libresmack
