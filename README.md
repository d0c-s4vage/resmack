# RESMACK

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
